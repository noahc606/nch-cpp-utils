#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include "MediaPlayer.h"
#include <nch/sdl-utils/Timer.h>
#include <nch/ffmpeg-utils/media/AudioUtils.h>
using namespace nch;

MediaPlayer::MediaPlayer(std::string url, SDL_Renderer* rend, int maxFramesToDecode, bool logInitInfo, bool logFrameInfo)
{
    mpd.url = url;
    mpd.renderer = rend;
    mpd.maxFramesToDecode = maxFramesToDecode;
    mpd.logInitInfo = logInitInfo;
    mpd.logFrameInfo = logFrameInfo;
    mpd.sdlAudioBufferSize = 1024;
}
MediaPlayer::MediaPlayer(std::string url, SDL_Renderer* rend) : MediaPlayer(url, rend, -1, false, false){}

MediaPlayer::~MediaPlayer()
{
    AudioUtils::quit = 1;
}

nch::MediaPlaybackData* MediaPlayer::getMediaPlaybackData() { return &mpd; }

AVFrame* MediaPlayer::getCurrentVidFrame()
{
    int size = mpd.vFrameCache.size();
    int cvfi = getCurrentVidFrameIndex();
    if(cvfi<size) {
        return mpd.vFrameCache[cvfi];
    }
    return nullptr;
}

int MediaPlayer::getCurrentVidFrameIndex(nch::MediaPlaybackData* mpd)
{
    /* Get current frame index */
    //Calculate current frame based on time & FPS
    uint64_t elapsedMS = Timer::getTicks64()-mpd->startTimeMS;
    int frame = ((double)elapsedMS/1000.0*mpd->fps);
    //If we are looping the video, make the frame index loop as well
    if(mpd->vFrameCacheComplete && mpd->infiniteLoop) {
        int vidNumFrames = mpd->vFrameCache.size();
        frame = frame%vidNumFrames;
    }
    if(frame<0) {
        frame = 0;
    }

    return frame;
}

int MediaPlayer::getCurrentVidFrameIndex() { return getCurrentVidFrameIndex(&mpd); }

void MediaPlayer::renderCurrentVidFrame(SDL_Rect* src, SDL_Rect* dst, const Color& colormod)
{    
    AVFrame* vFrame = getCurrentVidFrame();
    if(vFrame!=nullptr) {
        //Scale the image in pFrame->data and put the resulting scaled image into pict->data
        sws_scale(mpd.swsCtx, (uint8_t const* const*)vFrame->data, vFrame->linesize, 0, mpd.vCodecCtx->height, vFrame->data, vFrame->linesize);

        //Update the SDL texture with the new pict->data
        SDL_Rect rect;
        rect.x = 0; rect.y = 0; rect.w = mpd.vCodecCtx->width; rect.h = mpd.vCodecCtx->height;
        
        //SDL_Texture*
        SDL_UpdateYUVTexture(
            mpd.currVidTexture, &rect,
            vFrame->data[0], vFrame->linesize[0],
            vFrame->data[1], vFrame->linesize[1],
            vFrame->data[2], vFrame->linesize[2]
        );

        SDL_SetTextureBlendMode(mpd.currVidTexture, SDL_BLENDMODE_BLEND);
        SDL_SetTextureColorMod(mpd.currVidTexture, colormod.r, colormod.g, colormod.b);
        SDL_SetTextureAlphaMod(mpd.currVidTexture, colormod.a);
        SDL_RenderCopy(mpd.renderer, mpd.currVidTexture, src, dst);
    }
}

void MediaPlayer::renderCurrentVidFrame(SDL_Rect* src, SDL_Rect* dst) { renderCurrentVidFrame(src, dst, Color(255, 255, 255)); }

int MediaPlayer::decode(bool infiniteLoop)
{
    /* MediaPlaybackData parameters */
    mpd.infiniteLoop = infiniteLoop;

    /* [1] Setup FFMPEG's AVFormatContext and AV codecs; Setup SDL2 Audio. */
    //Initialize audio/video format context with URL and other info
    if(initAVFormatContext(&mpd.avFormatCtx, mpd.url, mpd.logInitInfo)!=0) {
        printf("Failed initAVFormatContext(), stopping.\n");
        return -1;
    }
    //Check audio/video format context for video and audio streams
    checkAVFormatContext(mpd.avFormatCtx, mpd.videoStream, mpd.audioStream);
    //Open audio/video Codecs
    openAVCodecs(mpd.avFormatCtx, mpd.videoStream, mpd.audioStream, &mpd.vCodec, &mpd.aCodec, &mpd.vCodecCtx, &mpd.aCodecCtx);
    //Open SDL audio device if an audio stream exists
    if(mpd.audioStream!=-1) openSDLAudioDevice(mpd.aCodecCtx, mpd.audioDeviceID, mpd.sdlAudioBufferSize);
    //Setup AVFrame* object
    setupAVFrame(mpd.currAVFrame);
    //Create texture for a rendering context
    if(mpd.videoStream!=-1) {
        mpd.currVidTexture = SDL_CreateTexture(
            mpd.renderer,
            SDL_PIXELFORMAT_YV12,
            SDL_TEXTUREACCESS_STREAMING,
            mpd.vCodecCtx->width,
            mpd.vCodecCtx->height
        );
    }

    //Setup AV Packet and SWS context
    setupAVPacket(mpd.currAVPacket);
    if(mpd.videoStream!=-1) setupSwsContext(mpd.swsCtx, mpd.vCodecCtx);


    /* [3] Packet decode loop - Add video frames to mpd.vFrameCache and add audio packets to AudioUtils' PacketQueue for playback */
    // read data from the AVFormatContext by repeatedly calling av_read_frame()
    uint64_t t0 = Timer::getTicks64();
    int ret = 0;
    int numFrames = 0;
    int numAudioPackets = 0;
    while(av_read_frame(mpd.avFormatCtx, mpd.currAVPacket)>=0 ) {
        //If we are looking at video data...
        if(mpd.currAVPacket->stream_index==mpd.videoStream) {
            //Send packet to vCodecCtx
            ret = avcodec_send_packet(mpd.vCodecCtx, mpd.currAVPacket);
            if(ret<0) {
                printf("Error sending packet for decoding.\n");
                return -1;
            }

            while(ret>=0) {
                //Decode frame
                ret = avcodec_receive_frame(mpd.vCodecCtx, mpd.currAVFrame);

                //Check for try-again/EOF/errors.
                if(ret==AVERROR(EAGAIN) || ret==AVERROR_EOF) {
                    break;
                } else if ( ret<0 ) {
                    printf("Error while decoding.\n");
                    return -1;
                }
                
                //Create a copy of this frame and add it to the vFrameCache
                AVFrame* newVFrame = av_frame_clone(mpd.currAVFrame);
                mpd.vFrameCache.push_back(newVFrame);
                numFrames++;
            }
        //If we are looking at audio data...
        } else if (mpd.currAVPacket->stream_index==mpd.audioStream) {
            // put the AVPacket in the audio PacketQueue
            AudioUtils::packet_queue_put(&AudioUtils::audioq, mpd.currAVPacket);
            numAudioPackets++;
        //If else, something is wrong...
        } else {
            av_packet_unref(mpd.currAVPacket);
            printf("Warning: Found invalid stream index '%d'...\n", mpd.currAVPacket->stream_index);
        }

        if(shouldQuit()) {
            return quit();
        }
    }

    /* Print info, mark complete, and return successful (0) after video decode finishes */
    uint64_t t1 = Timer::getTicks64()-t0;
    printf("Finished decoding video \"%s\" in %dms (%d frames, %d audio packets).\n", mpd.url.c_str(), t1, numFrames, numAudioPackets);
    mpd.vFrameCacheComplete = true;
    return 0;
}

void MediaPlayer::playback()
{
    /* Get FPS; Play audio stream; Get video start time. */
    //Get FPS
    if(mpd.videoStream!=-1) mpd.fps = av_q2d(mpd.avFormatCtx->streams[mpd.videoStream]->r_frame_rate);
    //Audio queue init and unpause mpd.audioDeviceID
    //AudioUtils::packet_queue_init(&AudioUtils::audioq);
    //SDL_PauseAudioDevice(mpd.audioDeviceID, 0);
    //Set mpd.startTimeMS
    mpd.startTimeMS = Timer::getTicks64();
}

int MediaPlayer::decode() { return decode(false); }

int MediaPlayer::initAVFormatContext(AVFormatContext** avfc, std::string url, bool dumpInfo)
{
    /* Open stream (1); check stream (2); dump stream's info (3). */
    //1 - Open an input stream specified by 'url', which will be stored at 'avFormatCtx'
    if (avformat_open_input(avfc, url.c_str(), NULL, NULL)<0) {
        printf("Could not open file \"%s\"\n", url.c_str());
        return -1;
    }
    //2 - Check if stream information from AVFormatContext* is OK
    if (avformat_find_stream_info(*avfc, NULL)<0) {
        printf("Could not find stream information from \"%s\"\n", url.c_str());
        return -2;
    }
    //3 - Print a bunch of info if 'dumpInfo' true
    if(dumpInfo) {
        printf("========[ av_dump_format - DUMP START ]========\n");
        av_dump_format(*avfc, 0, url.c_str(), 0);
        printf("========[ av_dump_format - DUMP END ]========\n");        
    }
    return 0;
}

void MediaPlayer::checkAVFormatContext(AVFormatContext* avfc, int& videoStream, int& audioStream)
{
    /* Check avFormatCtx */
    //Check avFormatCtx->streams[] array and make sure the video and audio streams exist.
    //The index of the corresponding codecs (0 or 1) will be stored into videoStream and audioStream.
    //If not, return -1.
    videoStream = -1;
    audioStream = -1;
    for(int i = 0; i<avfc->nb_streams; i++) {
        //check the General type of the encoded data to match AVMEDIA_TYPE_VIDEO
        if (avfc->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_VIDEO && videoStream<0) {
            videoStream = i;
        }
        //check the General type of the encoded data to match AVMEDIA_TYPE_AUDIO
        if (avfc->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_AUDIO && audioStream<0) {
            audioStream = i;
        }
    }
    //if (videoStream==-1) { printf("Note: No video stream found.\n"); }
    //if (audioStream==-1) { printf("Note: No audio stream found.\n");  }
}

void MediaPlayer::openAVCodecs(AVFormatContext* avfc, int videoStream, int audioStream, const AVCodec** vCodec, const AVCodec** aCodec, AVCodecContext** vCodecCtx, AVCodecContext** aCodecCtx)
{
    /*
        Find, allocate, and open the video & audio codecs.
        Find vCodec & aCodec (1); allocate vCodecCtx & aCodecCtx (2); open vCodec given the vCodecCtx; same with aCodec/aCodecCtx (3).
    */

    //1 - Find vCodec/aCodec if their streams exist
    if(videoStream!=-1) {
        *vCodec = avcodec_find_decoder(avfc->streams[videoStream]->codecpar->codec_id);
        if(vCodec==NULL) printf("Unsupported video codec!\n");
    }
        
    if(audioStream!=-1) {
        *aCodec = avcodec_find_decoder(avfc->streams[audioStream]->codecpar->codec_id);
        if(aCodec==NULL) printf("Unsupported audio codec!\n");
    }
    
    //2 - Allocate vCodecCtx/aCodecCtx
    if(videoStream!=-1) {
        *vCodecCtx = avcodec_alloc_context3(*vCodec);
        if(avcodec_parameters_to_context(*vCodecCtx, avfc->streams[videoStream]->codecpar)<0) {
            printf("Could not allocate video codec context.\n");
        }
    }
    if(audioStream!=-1) {
        *aCodecCtx = avcodec_alloc_context3(*aCodec);
        if(avcodec_parameters_to_context(*aCodecCtx, avfc->streams[audioStream]->codecpar)<0) {
            printf("Could not allocate audio codec context.\n");
        }
    }

    //3 - Open pCodec/aCodecCtx given its 'vCodecCtx'/'aCodecCtx' (codec context)
    if(videoStream!=-1) {
        if(avcodec_open2(*vCodecCtx, *vCodec, NULL)<0) {
            printf("Could not open video codec.\n");
        }
    }
    if(audioStream!=-1) {
        if(avcodec_open2(*aCodecCtx, *aCodec, NULL)<0) {
            printf("Could not open audio codec.\n");
        }
    }
}

void MediaPlayer::openSDLAudioDevice(AVCodecContext* aCodecCtx, SDL_AudioDeviceID& audioDeviceID, int sdlAudioBufferSize)
{
    // audio specs containers
    SDL_AudioSpec wanted_specs;
    SDL_AudioSpec specs;

    // set audio settings from codec info
    wanted_specs.freq = aCodecCtx->sample_rate;
    wanted_specs.format = AUDIO_S16SYS;
    wanted_specs.channels = aCodecCtx->channels;
    wanted_specs.silence = 0;
    wanted_specs.samples = sdlAudioBufferSize;
    wanted_specs.callback = AudioUtils::audio_callback;
    wanted_specs.userdata = aCodecCtx;

    // open audio device
    audioDeviceID = SDL_OpenAudioDevice(    // [1]
        NULL,
        0,
        &wanted_specs,
        &specs,
        SDL_AUDIO_ALLOW_FORMAT_CHANGE
    );

    // SDL_OpenAudioDevice returns a valid device ID that is > 0 on success or 0 on failure
    if(audioDeviceID==0) {
        printf("Failed to open audio device: %s.\n", SDL_GetError());
    }
}

void MediaPlayer::setupAVFrame(AVFrame*& avFrame)
{
    avFrame = av_frame_alloc();
    if(avFrame==NULL) {
        printf("Could not allocate frame.\n");
    }
}

void MediaPlayer::setupAVPacket(AVPacket*& avPacket)
{
    //Allocate an AVPacket to read data from the AVFormatContext
    avPacket = av_packet_alloc();
    if(!avPacket) {
        printf("Could not alloc packet.\n");
    }
}

void MediaPlayer::setupSwsContext(SwsContext*& swsCtx, AVCodecContext* vCodecCtx)
{
    // set up our SWSContext to convert the image data to YUV420
    swsCtx = sws_getContext(
        vCodecCtx->width,
        vCodecCtx->height,
        vCodecCtx->pix_fmt,
        vCodecCtx->width,
        vCodecCtx->height,
        AV_PIX_FMT_YUV420P,
        SWS_BILINEAR,
        NULL,
        NULL,
        NULL
    );
}

int MediaPlayer::playVideo(void* data)
{
    nch::MediaPlaybackData* mpd = (nch::MediaPlaybackData*)data;
    
    double fps = mpd->fps;
    printf("Running video thread (%f FPS)\n", fps);

    //If there is a video to be played, play the video. (For audio-only media, FPS is -1)
    if(fps>0) {
        int frame = 0;
        while(true) {
            int frame = getCurrentVidFrameIndex(mpd);
            if(mpd->vFrameCache.size()>frame) {
                AVFrame* vFrame = mpd->vFrameCache[frame];

                //Log info if enabled
                if(mpd->logFrameInfo) {
                    printf(
                        "Frame %c (%d) pts %d dts %d key_frame %d [coded_picture_number %d, display_picture_number %d, %dx%d]\n",
                        av_get_picture_type_char(vFrame->pict_type),
                        mpd->vCodecCtx->frame_number,
                        vFrame->pts, vFrame->pkt_dts, vFrame->key_frame, vFrame->coded_picture_number, vFrame->display_picture_number,
                        mpd->vCodecCtx->width, mpd->vCodecCtx->height
                    );
                }
            }

            if(shouldQuit()) {
                return quit();
            }
        }
    //If there is no video, hang until the audio stops or user quits.
    } else {
        while(!AudioUtils::quit) {
            if(shouldQuit()) {
                return quit();
            }
        }
    }
    return 1;
}

bool MediaPlayer::shouldQuit()
{
    // exit decoding loop if global quit flag is set
    if(AudioUtils::quit) {
        return true;
    }

    return false;
}

int MediaPlayer::quit()
{
    AudioUtils::quit = 1;
    return 0;
}

#pragma GCC diagnostic pop
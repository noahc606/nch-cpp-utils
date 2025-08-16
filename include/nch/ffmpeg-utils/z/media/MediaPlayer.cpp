#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include "MediaLoader.h"
#include "MediaPlayer.h"
#include "nch/cpp-utils/timer.h"
#include "nch/cpp-utils/log.h"
using namespace nch;

bool MediaPlayer::shouldPlaybackQuit = false;
bool MediaPlayer::logInitInfo = false;
bool MediaPlayer::logFrameInfo = false;

MediaPlayer::MediaPlayer(std::string url, SDL_Renderer* rend, int numCachedFrames)
{
    MediaPlayer::url = url;
    MediaPlayer::numCachedFrames = numCachedFrames;
    MediaPlayer::renderer = rend;

    mpd.sdlAudioBufferSize = 1024;
}
MediaPlayer::MediaPlayer(std::string url, SDL_Renderer* rend) : MediaPlayer(url, rend, -1){}

MediaPlayer::~MediaPlayer(){}

nch::MediaPlaybackData* MediaPlayer::getMediaPlaybackData() { return &mpd; }

AVFrame* MediaPlayer::getCurrentVidFrame()
{
    int cvfi = getCurrentVidFrameIndex();

    auto itr = mpd.vFrameCacheMap.find(cvfi);
    if(itr!=mpd.vFrameCacheMap.end()) {
        return itr->second;
    }

    return nullptr;
}

int MediaPlayer::getCurrentVidFrameIndex(nch::MediaPlaybackData* mpd)
{
    /* Get current frame index */
    //Calculate current frame based on time & FPS
    uint64_t elapsedMS = Timer::getTicks()-mpd->logicalStartTimeMS;
    int frame = ((double)elapsedMS/1000.0*mpd->fps);
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
        SDL_RenderCopy(renderer, mpd.currVidTexture, src, dst);
    }
}

void MediaPlayer::renderCurrentVidFrame(SDL_Rect* src, SDL_Rect* dst) { renderCurrentVidFrame(src, dst, Color(255, 255, 255)); }

bool MediaPlayer::initMediaPlaybackData()
{
    /* [1] Setup FFMPEG's AVFormatContext and AV codecs; Setup SDL2 Audio. */
    //Initialize audio/video format context with URL and other info
    if(MediaLoader::initAVFormatContext(mpd, url, logInitInfo)!=0) {
        printf("Failed initAVFormatContext(), stopping.\n");
        return false;
    }
    //Check audio/video format context for video and audio streams
    MediaLoader::checkAVFormatContext(mpd.avFormatCtx, mpd.videoStream, mpd.audioStream);
    //Open audio/video Codecs
    MediaLoader::openAVCodecs(mpd.avFormatCtx, mpd.videoStream, mpd.audioStream, &mpd.vCodec, &mpd.aCodec, &mpd.vCodecCtx, &mpd.aCodecCtx);
    //Open SDL audio device if an audio stream exists
    if(mpd.audioStream!=-1) MediaLoader::openSDLAudioDevice(mpd.aCodecCtx, mpd.audioDeviceID, mpd.sdlAudioBufferSize);
    //Get FPS
    if(mpd.videoStream!=-1) mpd.fps = av_q2d(mpd.avFormatCtx->streams[mpd.videoStream]->r_frame_rate);

    //Setup AVFrame* object
    MediaLoader::setupAVFrame(mpd.currAVFrame);
    //Create texture for a rendering context
    if(mpd.videoStream!=-1) {
        mpd.currVidTexture = SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_YV12,
            SDL_TEXTUREACCESS_STREAMING,
            mpd.vCodecCtx->width,
            mpd.vCodecCtx->height
        );
    }

    //Setup AV Packet and SWS context
    MediaLoader::setupAVPacket(mpd.currAVPacket);
    if(mpd.videoStream!=-1) {
        MediaLoader::setupSwsContext(mpd.swsCtx, mpd.vCodecCtx);
    }

    return true;
}

int MediaPlayer::startDecodingFrom(int startingVidFrameNum)
{
    initMediaPlaybackData();

    /* [3] Packet decode loop - Add video frames to mpd.vFrameCacheMap and add audio packets to AudioUtils' PacketQueue for playback */
    // read data from the AVFormatContext by repeatedly calling av_read_frame()
    uint64_t t0 = Timer::getTicks();
    int ret = 0;
    int numFramesCached = 0;
    int numAudioPackets = 0;
    int numVidFramesScanned = 0;

    bool finishedSkip = false;

    nch::Timer skipTime;

    while(av_read_frame(mpd.avFormatCtx, mpd.currAVPacket)>=0 && (numFramesCached<numCachedFrames||numCachedFrames==-1)) {
        
        if(mpd.currAVPacket->stream_index==mpd.videoStream) {
            numVidFramesScanned++;
        }
        
        if(numVidFramesScanned<startingVidFrameNum) {
            continue;
        } else if(!finishedSkip) {
            finishedSkip = true;

            mpd.logicalStartTimeMS = -1000.*((double)numVidFramesScanned)/mpd.fps;

            if(startingVidFrameNum!=0) {
                int skipTimeTotalSecs = ((double)numVidFramesScanned)/mpd.fps;
                int stHour = skipTimeTotalSecs/3600;
                int stMin = (skipTimeTotalSecs/60)%60;
                int stSec = skipTimeTotalSecs%60;

                nch::Log::log(
                    "Skipped to time %d:%d:%d (vidframe %d) in %fms.",
                    stHour, stMin, stSec, numVidFramesScanned, skipTime.getElapsedTimeMS()
                );
            }
        }

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
                
                //Create a copy of this frame and add it to the vFrameCacheMap
                AVFrame* newVFrame = av_frame_clone(mpd.currAVFrame);
                mpd.vFrameCacheMap.insert(std::make_pair(numVidFramesScanned, newVFrame));
                numFramesCached++;
            }
        //If we are looking at audio data...
        } else if (mpd.currAVPacket->stream_index==mpd.audioStream) {
            // put the AVPacket in the audio PacketQueue
            //audio...putPacketQueue(&AudioUtils::audioq, mpd.currAVPacket);
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
    uint64_t t1 = Timer::getTicks()-t0;

    
    nch::Log::log(
        "Decoded video \"%s\" in %dms (%d/%d frames, %d/-1 audio packets). Decode speed: %ffps",
        url.c_str(), t1, numFramesCached, numCachedFrames, numAudioPackets, (float)numFramesCached/(t1/1000.)
    );
    
    mpd.vFrameCacheComplete = true;
    return 0;
}

int MediaPlayer::startDecoding() { return startDecodingFrom(0); }

void MediaPlayer::startPlayback(bool infiniteLoop)
{
    /* Get video start time; Start video playback thread. */
    mpd.logicalStartTimeMS = Timer::getTicks();

    SDL_Thread* threadID = SDL_CreateThread(playVideo, "VideoThread", (void*)(&mpd));
}

void MediaPlayer::startPlayback() { startPlayback(false); }

int MediaPlayer::playVideo(void* data)
{
    nch::MediaPlaybackData* mpd = (nch::MediaPlaybackData*)data;

    /*
    
    double fps = mpd->fps;
    printf("Running video playback thread (%f FPS)\n", fps);

    //If there is a video to be played, play the video. (For audio-only media, FPS is -1)
    if(fps>0) {
        int frame = 0;
        while(true) {
            int frame = getCurrentVidFrameIndex(mpd);
            if(mpd->vFrameCache.size()>frame) {
                AVFrame* vFrame = mpd->vFrameCache[frame];

                //Log info if enabled
                if(logFrameInfo) {
                    int64_t framenum = -1;
                    #if ( LIBAVCODEC_VERSION_MAJOR>=61 )
                        framenum = mpd->vCodecCtx->frame_num;
                    #elif ( LIBAVCODEC_VERSION_MAJOR<=59 )
                        framenum = mpd->vCodecCtx->frame_number;
                    #endif

                    int vfdpn = -1;
                    #if( LIBAVUTIL_VERSION_MAJOR<=57 )
                        vfdpn = vFrame->display_picture_number;
                    #endif

                    int vfcpn = -1;
                    #if( LIBAVUTIL_VERSION_MAJOR<=57 )
                        vfcpn = vFrame->coded_picture_number;
                    #endif

                    printf(
                        "Frame %c (%d) pts %d dts %d key_frame %d [coded_picture_number %d, display_picture_number %d, %dx%d]\n",
                        av_get_picture_type_char(vFrame->pict_type),
                        framenum,
                        vFrame->pts, vFrame->pkt_dts, vFrame->key_frame, vfcpn, vfdpn,
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
        if(shouldQuit()) {
            return quit();
        }
    }
    return 1;*/
    return 1;
}

bool MediaPlayer::shouldQuit()
{
    return shouldPlaybackQuit;
}

int MediaPlayer::quit()
{
    shouldPlaybackQuit = true;
    return 0;
}

#pragma GCC diagnostic pop
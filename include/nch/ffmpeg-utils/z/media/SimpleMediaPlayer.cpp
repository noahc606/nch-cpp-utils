
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include "SimpleMediaPlayer.h"
#include "MediaLoader.h"
#include "nch/sdl-utils/timer.h"
#include "nch/cpp-utils/log.h"
using namespace nch;

SimpleMediaPlayer::SimpleMediaPlayer(std::string url, SDL_Renderer* rend, int maxFramesToDecode, bool logInitInfo, bool logFrameInfo)
{
    SimpleMediaPlayer::url = url;
    SimpleMediaPlayer::renderer = rend;
    SimpleMediaPlayer::maxFramesToDecode = maxFramesToDecode;
    SimpleMediaPlayer::logInitInfo = logInitInfo;    
    SimpleMediaPlayer::logFrameInfo = logFrameInfo;    

    mpd.sdlAudioBufferSize = 1024;
}
SimpleMediaPlayer::SimpleMediaPlayer(std::string url, SDL_Renderer* rend) : SimpleMediaPlayer(url, rend, -1, false, false){}

SimpleMediaPlayer::~SimpleMediaPlayer(){}

nch::MediaPlaybackData* SimpleMediaPlayer::getMediaPlaybackData() { return &mpd; }

AVFrame* SimpleMediaPlayer::getCurrentVidFrame()
{
    int size = mpd.vFrameCacheMap.size();
    int cvfi = getCurrentVidFrameIndex();
    if(cvfi<size) {
        return mpd.vFrameCacheMap[cvfi];
    }
    return nullptr;
}

int SimpleMediaPlayer::getCurrentVidFrameIndex(nch::MediaPlaybackData* mpd)
{
    /* Get current frame index */
    //Calculate current frame based on time & FPS
    uint64_t elapsedMS = Timer::getTicks64()-startTimeMS;
    int frame = ((double)elapsedMS/1000.0*mpd->fps);

    //Since we are looping the video, make the frame index loop as well
    if(mpd->vFrameCacheComplete) {
        int vidNumFrames = mpd->vFrameCacheMap.size();
        frame = frame%vidNumFrames;
    }
    if(frame<0) {
        frame = 0;
    }

    return frame;
}

int SimpleMediaPlayer::getCurrentVidFrameIndex() { return getCurrentVidFrameIndex(&mpd); }

void SimpleMediaPlayer::renderCurrentVidFrame(SDL_Rect* src, SDL_Rect* dst, const Color& colormod)
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

void SimpleMediaPlayer::renderCurrentVidFrame(SDL_Rect* src, SDL_Rect* dst) { renderCurrentVidFrame(src, dst, Color(255, 255, 255)); }

int SimpleMediaPlayer::decodeFull()
{
    

    /* [1] Setup FFMPEG's AVFormatContext and AV codecs; Setup SDL2 Audio. */
    //Initialize audio/video format context with URL and other info
    if(MediaLoader::initAVFormatContext(mpd, url, logInitInfo)!=0) {
        printf("Failed initAVFormatContext(), stopping.\n");
        return -1;
    }
    //Check audio/video format context for video and audio streams
    MediaLoader::checkAVFormatContext(mpd.avFormatCtx, mpd.videoStream, mpd.audioStream);
    //Open audio/video Codecs
    MediaLoader::openAVCodecs(mpd.avFormatCtx, mpd.videoStream, mpd.audioStream, &mpd.vCodec, &mpd.aCodec, &mpd.vCodecCtx, &mpd.aCodecCtx);
    
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
    if(mpd.videoStream!=-1) MediaLoader::setupSwsContext(mpd.swsCtx, mpd.vCodecCtx);


    /* [3] Packet decode loop - Add video frames to mpd.vFrameCache and add audio packets to AudioUtils' PacketQueue for playback */
    // read data from the AVFormatContext by repeatedly calling av_read_frame()
    uint64_t t0 = Timer::getTicks64();
    int ret = 0;
    int numVidFrames = 0;
    int numAudioPackets = 0;

    while(av_read_frame(mpd.avFormatCtx, mpd.currAVPacket)>=0 && (numVidFrames<maxFramesToDecode||maxFramesToDecode==-1)) {
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
                mpd.vFrameCacheMap.insert(std::make_pair(numVidFrames, newVFrame));
                numVidFrames++;
            }
        //If we are looking at audio data...
        } else if (mpd.currAVPacket->stream_index==mpd.audioStream) {
            //Do nothing
        //If else, something is wrong...
        } else {
            av_packet_unref(mpd.currAVPacket);
            printf("Warning: Found invalid stream index '%d'...\n", mpd.currAVPacket->stream_index);
        }

        if(shouldQuit()) {
            quit();
        }
    }

    /* Print info, mark complete, and return successful (0) after video decode finishes */
    uint64_t t1 = Timer::getTicks64()-t0;

    
    nch::Log::log(
        "Decoded video \"%s\" in %dms (%d/%d frames, %d/-1 audio packets). Decode speed: %ffps",
        url.c_str(), t1, numVidFrames, maxFramesToDecode, numAudioPackets, (float)numVidFrames/(t1/1000.)
    );

    mpd.vFrameCacheComplete = true;
    return 0;
}

void SimpleMediaPlayer::startPlayback()
{
    if(mpd.videoStream!=-1) mpd.fps = av_q2d(mpd.avFormatCtx->streams[mpd.videoStream]->r_frame_rate);
    startTimeMS = Timer::getTicks64();
}

bool SimpleMediaPlayer::shouldQuit() { return !running; }
void SimpleMediaPlayer::quit() { running = false; }

#pragma GCC diagnostic pop
#pragma once
#include <SDL2/SDL.h>
#include <string>
#include <vector>
extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavutil/imgutils.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libswresample/swresample.h>
}
#include <nch/cpp-utils/gfx/Color.h>
#include "MediaPlaybackData.h"

namespace nch { class MediaPlayer
{
public:
    MediaPlayer(std::string url, SDL_Renderer* rend, int maxFramesToDecode, bool logInitInfo, bool logFrameInfo);
    MediaPlayer(std::string url, SDL_Renderer* rend);
    ~MediaPlayer();

    nch::MediaPlaybackData* getMediaPlaybackData();
    AVFrame* getCurrentVidFrame();
    int getCurrentVidFrameIndex();
    static int getCurrentVidFrameIndex(nch::MediaPlaybackData* mpd);
    void renderCurrentVidFrame(SDL_Rect* src, SDL_Rect* dst, const Color& colormod);
    void renderCurrentVidFrame(SDL_Rect* src, SDL_Rect* dst);
    int decodeFull();
    void startPlayback(bool infiniteLoop);
    void startPlayback();


private:
    static int initAVFormatContext(AVFormatContext** avfc, std::string url, bool dumpInfo);
    static void checkAVFormatContext(AVFormatContext* avfc, int& videoStream, int& audioStream);
    static void openAVCodecs(AVFormatContext* avfc, int videoStream, int audioStream, const AVCodec** vCodec, const AVCodec** aCodec, AVCodecContext** vCodecCtx, AVCodecContext** aCodecCtx);
    static void openSDLAudioDevice(AVCodecContext* aCodecCtx, SDL_AudioDeviceID& audioDeviceID, int sdlAudioBufferSize);
    static void setupAVFrame(AVFrame*& avPacket);
    static void setupAVPacket(AVPacket*& avPacket);
    static void setupSwsContext(SwsContext*& swsCtx, AVCodecContext* vCodecCtx);

    static int playVideo(void* data);
    static int playAudio(void* data);
    static bool shouldQuit();
    static int quit();

    nch::MediaPlaybackData mpd;
    SDL_Thread* videoThread = nullptr;
};
}
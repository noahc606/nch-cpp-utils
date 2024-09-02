#pragma once
extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavutil/imgutils.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libswresample/swresample.h>
}
#include <string>
#include <SDL2/SDL.h>

namespace nch {
class MediaPlaybackData {
public:
    MediaPlaybackData(){}
    ~MediaPlaybackData(){}

    //Playback parameters
    int maxFramesToDecode = 10000;
    std::string url = "???null???";
    //Debugging parameters
    bool logInitInfo = false;
    bool logFrameInfo = false;


    SDL_Renderer* renderer = nullptr;
    double fps = -1;

    int sdlAudioBufferSize = -1;
    uint64_t startTimeMS = 0;
    bool infiniteLoop = false;

    AVFormatContext* avFormatCtx = nullptr;
    int videoStream = -1;
    int audioStream = -1;
    const AVCodec* vCodec = nullptr;
    const AVCodec* aCodec = nullptr;
    AVCodecContext* vCodecCtx = nullptr;
    AVCodecContext* aCodecCtx = nullptr;

    AVFrame* currAVFrame = nullptr;
    AVFrame* pict = nullptr;
    AVPacket* currAVPacket = nullptr;
    SDL_Texture* currVidTexture = nullptr;
    struct SwsContext* swsCtx = nullptr;
    std::vector<AVFrame*> vFrameCache;
    bool vFrameCacheComplete = false;

    SDL_AudioDeviceID audioDeviceID = 0;


private:

};
}
#pragma once
extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavutil/imgutils.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libswresample/swresample.h>
}
#include <map>
#include <string>
#include <SDL2/SDL.h>

namespace nch { class MediaPlaybackData {
public:
    MediaPlaybackData(){}
    ~MediaPlaybackData(){}

    double fps = -1;

    int sdlAudioBufferSize = -1;
    uint64_t logicalStartTimeMS = 0;

    AVFormatContext* avFormatCtx = NULL;
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
    std::map<int, AVFrame*> vFrameCacheMap;
    bool vFrameCacheComplete = false;

    SDL_AudioDeviceID audioDeviceID = 0;


private:

};
}
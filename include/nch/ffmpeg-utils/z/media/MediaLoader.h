#pragma once
#include "MediaPlaybackData.h"

namespace nch {
class MediaLoader {
public:
    MediaLoader();
    ~MediaLoader();

    static int initAVFormatContext(nch::MediaPlaybackData& mpd, std::string url, bool logInitInfo);
    static void checkAVFormatContext(AVFormatContext* avfc, int& videoStream, int& audioStream);
    static void openAVCodecs(AVFormatContext* avfc, int videoStream, int audioStream, const AVCodec** vCodec, const AVCodec** aCodec, AVCodecContext** vCodecCtx, AVCodecContext** aCodecCtx);
    static void openSDLAudioDevice(AVCodecContext* aCodecCtx, SDL_AudioDeviceID& audioDeviceID, int sdlAudioBufferSize);
    static void setupAVFrame(AVFrame*& avPacket);
    static void setupAVPacket(AVPacket*& avPacket);
    static void setupSwsContext(SwsContext*& swsCtx, AVCodecContext* vCodecCtx);

private:
};
}
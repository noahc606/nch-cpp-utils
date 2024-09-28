#pragma once

#include <SDL2/SDL.h>
extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavutil/imgutils.h>
    #include <libavutil/opt.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libswresample/swresample.h>
}

namespace nch { class AudioUtils
{
public:
    typedef struct PacketQueue
    {
        AVPacketList* first_pkt;
        AVPacketList* last_pkt;
        int nb_packets;
        int size;
        SDL_mutex* mutex;
        SDL_cond* cond;
    } PacketQueue;

    AudioUtils(/* args */);
    ~AudioUtils();

    static void initPacketQueue(PacketQueue* q);
    static int putPacketQueue(PacketQueue* q, AVPacket* pkt);
    static void audioCallback(void* userdata, Uint8* stream, int len);

    static PacketQueue audioq;
    static int quit;

private:
    static int getPacketQueue(PacketQueue* q, AVPacket* pkt, int block);
    static int audioDecodeFrame(AVCodecContext* aCodecCtx, uint8_t* audio_buf, int buf_size);
    static int audioResampling(AVCodecContext* audio_decode_ctx, AVFrame* decoded_audio_frame, enum AVSampleFormat out_sample_fmt, int out_channels, int out_sample_rate, uint8_t* out_buf);

};
}
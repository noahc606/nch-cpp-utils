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

    static void packet_queue_init(PacketQueue* q);
    static int packet_queue_put(PacketQueue* q, AVPacket* pkt);
    static int packet_queue_get(PacketQueue* q, AVPacket* pkt, int block);
    static void audio_callback(void* userdata, Uint8* stream, int len);
    static int audio_decode_frame(AVCodecContext* aCodecCtx, uint8_t* audio_buf, int buf_size);
    static int audio_resampling(AVCodecContext* audio_decode_ctx, AVFrame* decoded_audio_frame, enum AVSampleFormat out_sample_fmt, int out_channels, int out_sample_rate, uint8_t* out_buf);

    // audio PacketQueue instance
    static PacketQueue audioq;
    // global quit flag
    static int quit;

private:

};
}
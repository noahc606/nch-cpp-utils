#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include "AudioUtils.h"
#include <assert.h>

#define MAX_AUDIO_FRAME_SIZE 192000

using namespace nch;

int AudioUtils::quit = 0;
AudioUtils::PacketQueue AudioUtils::audioq;

void AudioUtils::initPacketQueue(PacketQueue* q)
{
    //Zero-out memory pointed to by 'q'
    memset(q, 0, sizeof(PacketQueue));
    
    //Create thread mutex or exit on failure
    q->mutex = SDL_CreateMutex();
    if(!q->mutex) {
        printf("SDL_CreateMutex Error: %s.\n", SDL_GetError());
        return;
    }

    //Create condition variable or exit on failure
    q->cond = SDL_CreateCond();
    if(!q->cond) {
        printf("SDL_CreateCond Error: %s.\n", SDL_GetError());
        return;
    }
}

int AudioUtils::putPacketQueue(PacketQueue* q, AVPacket* pkt)
{
    //From 'pkt', create an AVPacketList to be put into 'q'.
    AVPacketList* avPacketList = (AVPacketList*)av_malloc(sizeof(AVPacketList));
    if (!avPacketList) {
        return -1;
    }

    //Build 'avPacketList'
    avPacketList->pkt = *pkt;
    avPacketList->next = NULL;  //->next = end of queue

    /* Work with the PacketQueue 'q' */
    //Lock Mutex
    SDL_LockMutex(q->mutex);
    //Build 'q'
    if(!q->last_pkt) {                      //If queue is empty...
        q->first_pkt = avPacketList;        //-> Insert as first
    } else {                                //If queue unempty...
        q->last_pkt->next = avPacketList;   //-> Insert as last
    }
    q->last_pkt = avPacketList;             //Last AVPacketList in the queue is the new 'avPacketList'
    q->nb_packets++;                        //Update number of packets
    q->size += avPacketList->pkt.size;      //Update total size of the queue
    //Notify getPacketQueue which is waiting that a new packet is available
    SDL_CondSignal(q->cond);
    //Unlock Mutex
    SDL_UnlockMutex(q->mutex);

    return 0;
}

void AudioUtils::audioCallback(void* userdata, Uint8* stream, int len)
{
    //Retrieve the audio codec context
    AVCodecContext* aCodecCtx = (AVCodecContext*)userdata;
    int len1 = -1;
    int audio_size = -1;

    // The size of audio_buf is 1.5 times the size of the largest audio frame
    // that FFmpeg will give us, which gives us a nice cushion.
    static uint8_t audio_buf[(MAX_AUDIO_FRAME_SIZE * 3) / 2];
    static unsigned int audio_buf_size = 0;
    static unsigned int audio_buf_index = 0;

    while(len>0) {
        if(quit) { return; }

        if(audio_buf_index>=audio_buf_size) {
            //We have already sent all avaialble data; get more
            audio_size = audioDecodeFrame(aCodecCtx, audio_buf, sizeof(audio_buf));
            if(audio_size<0) {
                //Output silence...
                audio_buf_size = 1024;
                //Clear memory
                memset(audio_buf, 0, audio_buf_size);
                printf("audio_decode_frame() failed.\n");
            } else {
                audio_buf_size = audio_size;
            }

            audio_buf_index = 0;
        }

        len1 = audio_buf_size - audio_buf_index;
        if (len1 > len) { len1 = len; }

        // copy data from audio buffer to the SDL stream
        memcpy(stream, (uint8_t*)audio_buf + audio_buf_index, len1);

        len -= len1;
        stream += len1;
        audio_buf_index += len1;
    }
}





int AudioUtils::getPacketQueue(PacketQueue* q, AVPacket* pkt, int block)
{
    int ret;
    AVPacketList* avPacketList;

    // lock mutex
    SDL_LockMutex(q->mutex);

    for (;;) {
        // check quit flag
        if(quit) {
            ret = -1;
            break;
        }

        // point to the first AVPacketList in the queue
        avPacketList = q->first_pkt;

        // if the first packet is not NULL, the queue is not empty
        if (avPacketList) {
            // place the second packet in the queue at first position
            q->first_pkt = avPacketList->next;

            // check if queue is empty after removal
            if (!q->first_pkt) {
                // first_pkt = last_pkt = NULL = empty queue
                q->last_pkt = NULL;
            }

            // decrease the number of packets in the queue
            q->nb_packets--;

            // decrease the size of the packets in the queue
            q->size -= avPacketList->pkt.size;

            // point pkt to the extracted packet, this will return to the calling function
            *pkt = avPacketList->pkt;

            // free memory
            av_free(avPacketList);

            ret = 1;
            break;
        } else if (!block) {
            ret = 0;
            break;
        } else {
            // unlock mutex and wait for cond signal, then lock mutex again
            SDL_CondWait(q->cond, q->mutex);
        }
    }

    // unlock mutex
    SDL_UnlockMutex(q->mutex);

    return ret;
}

int AudioUtils::audioDecodeFrame(AVCodecContext* aCodecCtx, uint8_t* audio_buf, int buf_size)
{
    AVPacket * avPacket = av_packet_alloc();
    static uint8_t * audio_pkt_data = NULL;
    static int audio_pkt_size = 0;

    // allocate a new frame, used to decode audio packets
    static AVFrame * avFrame = NULL;
    avFrame = av_frame_alloc();
    if (!avFrame) {
        printf("Could not allocate AVFrame.\n");
        return -1;
    }

    int len1 = 0;
    int data_size = 0;

    for (;;) {
        // check global quit flag
        if (quit) {
            return -1;
        }

        while (audio_pkt_size > 0) {
            int got_frame = 0;

            // [5]
            // len1 = avcodec_decode_audio4(aCodecCtx, avFrame, &got_frame, avPacket);
            int ret = avcodec_receive_frame(aCodecCtx, avFrame);
            if (ret==0)                 got_frame = 1;
            if (ret==AVERROR(EAGAIN))   ret = 0;
            if (ret==0)                 ret = avcodec_send_packet(aCodecCtx, avPacket);

            if (ret==AVERROR(EAGAIN)) {
                ret = 0;
            } else if (ret < 0) {
                printf("avcodec_receive_frame error");
                return -1;
            } else {
                len1 = avPacket->size;
            }

            if (len1 < 0) {
                // if error, skip frame
                audio_pkt_size = 0;
                break;
            }

            audio_pkt_data += len1;
            audio_pkt_size -= len1;
            data_size = 0;

            if(got_frame) {
                data_size = audioResampling( aCodecCtx, avFrame, AV_SAMPLE_FMT_S16, aCodecCtx->channels, aCodecCtx->sample_rate, audio_buf );
                assert(data_size <= buf_size);
            }

            if(data_size <= 0) {
                continue;
            }

            return data_size;
        }

        if(avPacket->data) {
            av_packet_unref(avPacket);
        }

        // get more audio AVPacket
        int ret = getPacketQueue(&audioq, avPacket, 1);
        if(ret<0) {
            return -1;
        }

        audio_pkt_data = avPacket->data;
        audio_pkt_size = avPacket->size;
    }

    return 0;
}

int AudioUtils::audioResampling(AVCodecContext* audio_decode_ctx, AVFrame* decoded_audio_frame, enum AVSampleFormat out_sample_fmt, int out_channels, int out_sample_rate, uint8_t* out_buf)
{
    //Check global quit flag
    if (quit) { return -1; }

    SwrContext * swr_ctx = NULL;
    int ret = 0;
    int64_t in_channel_layout = audio_decode_ctx->channel_layout;
    int64_t out_channel_layout = AV_CH_LAYOUT_STEREO;
    int out_nb_channels = 0;
    int out_linesize = 0;
    int in_nb_samples = 0;
    int out_nb_samples = 0;
    int max_out_nb_samples = 0;
    uint8_t ** resampled_data = NULL;
    int resampled_data_size = 0;

    swr_ctx = swr_alloc();

    if (!swr_ctx) {
        printf("swr_alloc error.\n");
        return -1;
    }

    // get input audio channels
    in_channel_layout = (audio_decode_ctx->channels ==
                     av_get_channel_layout_nb_channels(audio_decode_ctx->channel_layout)) ?   // 2
                     audio_decode_ctx->channel_layout :
                     av_get_default_channel_layout(audio_decode_ctx->channels);

    // check input audio channels correctly retrieved
    if (in_channel_layout <= 0) {
        printf("in_channel_layout error.\n");
        return -1;
    }

    // set output audio channels based on the input audio channels
    if (out_channels == 1) {
        out_channel_layout = AV_CH_LAYOUT_MONO;
    } else if (out_channels == 2) {
        out_channel_layout = AV_CH_LAYOUT_STEREO;
    } else {
        out_channel_layout = AV_CH_LAYOUT_SURROUND;
    }

    // retrieve number of audio samples (per channel)
    in_nb_samples = decoded_audio_frame->nb_samples;
    if (in_nb_samples <= 0) {
        printf("in_nb_samples error.\n");
        return -1;
    }

    // Set SwrContext parameters for resampling
    av_opt_set_int(   // 3
        swr_ctx,
        "in_channel_layout",
        in_channel_layout,
        0
    );

    // Set SwrContext parameters for resampling
    av_opt_set_int(
        swr_ctx,
        "in_sample_rate",
        audio_decode_ctx->sample_rate,
        0
    );

    // Set SwrContext parameters for resampling
    av_opt_set_sample_fmt(
        swr_ctx,
        "in_sample_fmt",
        audio_decode_ctx->sample_fmt,
        0
    );

    // Set SwrContext parameters for resampling
    av_opt_set_int(
        swr_ctx,
        "out_channel_layout",
        out_channel_layout,
        0
    );

    // Set SwrContext parameters for resampling
    av_opt_set_int(
        swr_ctx,
        "out_sample_rate",
        out_sample_rate,
        0
    );

    // Set SwrContext parameters for resampling
    av_opt_set_sample_fmt(
        swr_ctx,
        "out_sample_fmt",
        out_sample_fmt,
        0
    );

    // Once all values have been set for the SwrContext, it must be initialized
    // with swr_init().
    ret = swr_init(swr_ctx);;
    if (ret < 0) {
        printf("Failed to initialize the resampling context.\n");
        return -1;
    }

    max_out_nb_samples = out_nb_samples = av_rescale_rnd( in_nb_samples, out_sample_rate, audio_decode_ctx->sample_rate, AV_ROUND_UP );

    // check rescaling was successful
    if (max_out_nb_samples <= 0) {
        printf("av_rescale_rnd error.\n");
        return -1;
    }

    // get number of output audio channels
    out_nb_channels = av_get_channel_layout_nb_channels(out_channel_layout);

    ret = av_samples_alloc_array_and_samples( &resampled_data, &out_linesize, out_nb_channels, out_nb_samples, out_sample_fmt, 0 );
    if (ret < 0) {
        printf("av_samples_alloc_array_and_samples() error: Could not allocate destination samples.\n");
        return -1;
    }

    // retrieve output samples number taking into account the progressive delay
    out_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, audio_decode_ctx->sample_rate) + in_nb_samples, out_sample_rate, audio_decode_ctx->sample_rate, AV_ROUND_UP);
    if (out_nb_samples <= 0) {
        printf("av_rescale_rnd error\n");
        return -1;
    }

    if (out_nb_samples>max_out_nb_samples) {
        // free memory block and set pointer to NULL
        av_free(resampled_data[0]);

        // Allocate a samples buffer for out_nb_samples samples
        ret = av_samples_alloc(
                  resampled_data,
                  &out_linesize,
                  out_nb_channels,
                  out_nb_samples,
                  out_sample_fmt,
                  1
              );

        // check samples buffer correctly allocated
        if (ret < 0) {
            printf("av_samples_alloc failed.\n");
            return -1;
        }

        max_out_nb_samples = out_nb_samples;
    }

    if (swr_ctx) {
        // do the actual audio data resampling
        ret = swr_convert(
                  swr_ctx,
                  resampled_data,
                  out_nb_samples,
                  (const uint8_t **) decoded_audio_frame->data,
                  decoded_audio_frame->nb_samples
              );

        // check audio conversion was successful
        if (ret < 0) {
            printf("swr_convert_error.\n");
            return -1;
        }

        // Get the required buffer size for the given audio parameters
        resampled_data_size = av_samples_get_buffer_size(
                                  &out_linesize,
                                  out_nb_channels,
                                  ret,
                                  out_sample_fmt,
                                  1
                              );

        // check audio buffer size
        if (resampled_data_size < 0) {
            printf("av_samples_get_buffer_size error.\n");
            return -1;
        }
    } else {
        printf("swr_ctx null error.\n");
        return -1;
    }

    // copy the resampled data to the output buffer
    memcpy(out_buf, resampled_data[0], resampled_data_size);

    /*
     * Memory Cleanup.
     */
    if (resampled_data)  {
        // free memory block and set pointer to NULL
        av_freep(&resampled_data[0]);
    }

    av_freep(&resampled_data);
    resampled_data = NULL;

    if (swr_ctx) {
        // Free the given SwrContext and set the pointer to NULL
        swr_free(&swr_ctx);
    }

    return resampled_data_size;
}

#pragma GCC diagnostic pop
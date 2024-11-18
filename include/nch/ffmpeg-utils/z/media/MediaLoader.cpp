#include "MediaLoader.h"
using namespace nch;

int MediaLoader::initAVFormatContext(nch::MediaPlaybackData& mpd, std::string url, bool logInitInfo)
{
    /* Open stream (1); check stream (2); dump stream's info (3). */

    //1 - Open an input stream specified by 'url', which will be stored at 'avFormatCtx'
    mpd.avFormatCtx = NULL;
    if (avformat_open_input(&mpd.avFormatCtx, url.c_str(), NULL, NULL)<0) {
        printf("Could not open file \"%s\"\n", url.c_str());
        return -1;
    }
    //2 - Check if stream information from AVFormatContext* is OK
    if (avformat_find_stream_info(mpd.avFormatCtx, NULL)<0) {
        printf("Could not find stream information from \"%s\"\n", url.c_str());
        return -2;
    }
    //3 - Print a bunch of info if 'dumpInfo' true
    if(logInitInfo) {
        printf("========[ av_dump_format - DUMP START ]========\n");
        av_dump_format(mpd.avFormatCtx, 0, url.c_str(), 0);
        printf("========[ av_dump_format - DUMP END ]========\n");        
    }
    return 0;
}

void MediaLoader::checkAVFormatContext(AVFormatContext* avfc, int& videoStream, int& audioStream)
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

void MediaLoader::openAVCodecs(AVFormatContext* avfc, int videoStream, int audioStream, const AVCodec** vCodec, const AVCodec** aCodec, AVCodecContext** vCodecCtx, AVCodecContext** aCodecCtx)
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

void MediaLoader::openSDLAudioDevice(AVCodecContext* aCodecCtx, SDL_AudioDeviceID& audioDeviceID, int sdlAudioBufferSize)
{
    // audio specs containers
    SDL_AudioSpec wanted_specs;
    SDL_AudioSpec specs;

    // set audio settings from codec info
    wanted_specs.freq = aCodecCtx->sample_rate;
    wanted_specs.format = AUDIO_S16SYS;
    wanted_specs.channels = aCodecCtx->ch_layout.nb_channels;
    wanted_specs.silence = 0;
    wanted_specs.samples = sdlAudioBufferSize;
    //wanted_specs.callback = Audio...audioCallback;
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

void MediaLoader::setupAVFrame(AVFrame*& avFrame)
{
    avFrame = av_frame_alloc();
    if(avFrame==NULL) {
        printf("Could not allocate AVFrame.\n");
    }
}

void MediaLoader::setupAVPacket(AVPacket*& avPacket)
{
    //Allocate an AVPacket to read data from the AVFormatContext
    avPacket = av_packet_alloc();
    if(!avPacket) {
        printf("Could not alloc packet.\n");
    }
}

void MediaLoader::setupSwsContext(SwsContext*& swsCtx, AVCodecContext* vCodecCtx)
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
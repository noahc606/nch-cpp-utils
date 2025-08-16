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
#include "nch/cpp-utils/color.h"
#include "MediaPlaybackData.h"

namespace nch { class MediaPlayer
{
public:
    MediaPlayer(std::string url, SDL_Renderer* rend, int numCachedFrames);
    MediaPlayer(std::string url, SDL_Renderer* rend);
    ~MediaPlayer();

    nch::MediaPlaybackData* getMediaPlaybackData();
    AVFrame* getCurrentVidFrame();
    int getCurrentVidFrameIndex();
    static int getCurrentVidFrameIndex(nch::MediaPlaybackData* mpd);
    void renderCurrentVidFrame(SDL_Rect* src, SDL_Rect* dst, const Color& colormod);
    void renderCurrentVidFrame(SDL_Rect* src, SDL_Rect* dst);

    bool initMediaPlaybackData();
    int startDecodingFrom(int startingVidFrameNum);
    int startDecoding();
    void startPlayback(bool infiniteLoop);
    void startPlayback();


private:

    static int playVideo(void* data);
    static int playAudio(void* data);
    static bool shouldQuit();
    static int quit();

    nch::MediaPlaybackData mpd;
    SDL_Thread* videoThread = nullptr;
    SDL_Renderer* renderer = nullptr;
    
    //Playback parameters
    int numCachedFrames = 1000;
    std::string url = "???null???";
    //Debugging parameters
    static bool logInitInfo;
    static bool logFrameInfo;
    static bool shouldPlaybackQuit;
};
}
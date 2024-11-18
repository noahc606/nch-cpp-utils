#pragma once
#include <SDL2/SDL.h>
#include "MediaPlaybackData.h"
#include "Color.h"

namespace nch { class SimpleMediaPlayer {
public:
    SimpleMediaPlayer(std::string url, SDL_Renderer* rend, int maxFramesToDecode, bool logInitInfo, bool logFrameInfo);
    SimpleMediaPlayer(std::string url, SDL_Renderer* rend);
    ~SimpleMediaPlayer();

    nch::MediaPlaybackData* getMediaPlaybackData();
    AVFrame* getCurrentVidFrame();
    int getCurrentVidFrameIndex(nch::MediaPlaybackData* mpd);
    int getCurrentVidFrameIndex();
    bool shouldQuit();

    int decodeFull();
    void startPlayback();
    void quit();
    void renderCurrentVidFrame(SDL_Rect* src, SDL_Rect* dst, const Color& colormod);
    void renderCurrentVidFrame(SDL_Rect* src, SDL_Rect* dst);
private:
    nch::MediaPlaybackData mpd;
    SDL_Renderer* renderer;
    std::string url = "???null???";
    int maxFramesToDecode = -1;
    bool logInitInfo = false;
    bool logFrameInfo = false;

    bool running = true;
    uint64_t startTimeMS = 0;
    
}; }
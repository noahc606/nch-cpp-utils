#pragma once
#include <SDL2/SDL.h>
#include <vector>

class NCH_MainLoopDriver {
public:
    NCH_MainLoopDriver(SDL_Renderer*, void (*tickFunc)(), uint64_t targetTPS, void (*drawFunc)(SDL_Renderer*), uint64_t targetFPS);

private:
    void events();
    uint64_t getAvgNSPT();
    uint64_t getAvgNSPF();

    bool running = true;
    uint64_t secLast = 0;

    bool loggingPerformance = true;
    
    void (*tickFunc)(); void (*drawFunc)(SDL_Renderer*);
    int currentTPS = -1; int currentFPS = -1;
    int hardMaxFPS = 300;
    std::vector<uint64_t> tickTimesNS; std::vector<uint64_t> frameTimesNS;    
};
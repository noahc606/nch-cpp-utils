#pragma once
#include <SDL2/SDL.h>
#include <string>
#include <vector>

namespace nch { class MainLoopDriver {
public:
    MainLoopDriver(SDL_Renderer*, void (*tickFunc)(), uint64_t targetTPS, void (*drawFunc)(SDL_Renderer*), uint64_t targetFPS);

    static std::string getPerformanceInfo();
    static void quit();
private:
    void events();
    uint64_t getAvgNSPT();
    uint64_t getAvgNSPF();

    static bool running;
    uint64_t secLast = 0;

    bool loggingPerformance = false;
    static std::string performanceInfo;
    
    void (*tickFunc)(); void (*drawFunc)(SDL_Renderer*);
    int currentTPS = -1; int currentFPS = -1;
    int minTargetFPS = 20;
    int maxTargetFPS = 20;
    std::vector<uint64_t> tickTimesNS; std::vector<uint64_t> frameTimesNS;    
};}
#pragma once
#include <mutex>
#include <SDL2/SDL.h>
#include <string>
#include <vector>

namespace nch { class MainLoopDriver {
public:
    MainLoopDriver(SDL_Renderer*, void (*tickFunc)(), uint64_t targetTPS, void (*drawFunc)(SDL_Renderer*), uint64_t targetFPS, void (*eventFunc)(SDL_Event&));
    MainLoopDriver(SDL_Renderer*, void (*tickFunc)(), uint64_t targetTPS, void (*drawFunc)(SDL_Renderer*), uint64_t targetFPS);
    MainLoopDriver(void (*tickFunc)(), uint64_t targetTPS, void (*altDrawFunc)(), uint64_t targetFPS, void (*eventFunc)(SDL_Event&));
    MainLoopDriver(void (*tickFunc)(), uint64_t targetTPS, void (*altDrawFunc)(), uint64_t targetFPS);
    
    static int getCurrentTPS();
    static int getCurrentFPS();
    static std::string getPerformanceInfo();
    static uint64_t getNumTicksPassedTotal();

    static void quit();
private:
    void start(SDL_Renderer* rend, void (*tickFunc)(), uint64_t targetTPS, void (*drawFunc)(SDL_Renderer*), void (*altDrawFunc)(), uint64_t targetFPS, void (*eventFunc)(SDL_Event&));


    static void ticker();
    void events();

    //Main loop states
    static bool mldExists;
    static bool running;
    static int targetTPS; static int targetFPS;
    static uint64_t numTicksPassedTotal;
    //Debug info
    static bool loggingPerformance;
    static std::string performanceInfo;
    static int currentTPS; static int currentFPS;
    //Objects used by ticker
	static std::mutex mtx;
	static int currentNumTicksLeft;
	static uint64_t lastTickNS;
    //Draw, tick, event callbacks
    void (*tickFunc)() = nullptr;
    void (*altDrawFunc)() = nullptr;
    void (*drawFunc)(SDL_Renderer*) = nullptr;
    void (*eventFunc)(SDL_Event&) = nullptr;
};}
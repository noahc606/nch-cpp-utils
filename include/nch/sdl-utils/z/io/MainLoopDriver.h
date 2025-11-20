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
    
    static uint64_t getTargetNSPT();
    static uint64_t getTargetNSPF();
    static int getCurrentTPS();
    static int getCurrentFPS();
    static std::string getPerformanceInfo();
    static uint64_t getNumTicksPassedTotal();
    static bool hasQuit();
    
    static void drawPerformanceBenchmark(SDL_Renderer* sdlRend, int bmHeight, int windowWidth, int windowHeight);
    static void quit();
private:
    void start(SDL_Renderer* rend, void (*tickFunc)(), uint64_t targetTPS, void (*drawFunc)(SDL_Renderer*), void (*altDrawFunc)(), uint64_t targetFPS, void (*eventFunc)(SDL_Event&));
    static void mainLoop();

    static void ticker();
    static void events();

    //Main loop states
    static bool mldExists;
    static bool running;
    static int targetTPS; static int targetFPS;
    static uint64_t numTicksPassedTotal;
	//mainloop() helper variables
	static uint64_t nsPerFrame;
	static int fps, tps;
	static uint64_t nextFrameNS;
	static uint64_t numTicksPassedThisSec;
    static SDL_Renderer* rend;
    //Debug stuff
    static bool loggingPerformance;
    static std::string performanceInfo;
    static int currentTPS; static int currentFPS;
    static std::vector<double> frameTimes, tickTimes;
    //Objects used by ticker
	static std::mutex mtx;
	static int currentNumTicksLeft;
	static uint64_t lastTickNS;
    static bool manualTicker;
	static uint64_t numTicksPassed;	//Number of ticks that should have passed according to time since launch
	static uint64_t nextTickNS;		//Time of the next tick

    //Draw, tick, event callbacks
    static void (*tickFunc)();
    static void (*altDrawFunc)();
    static void (*drawFunc)(SDL_Renderer*);
    static void (*eventFunc)(SDL_Event&);
};}
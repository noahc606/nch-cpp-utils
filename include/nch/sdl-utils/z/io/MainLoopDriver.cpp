#include "MainLoopDriver.h"
#include <SDL2/SDL_timer.h>
#include <thread>
#include "nch/cpp-utils/log.h"
#include "nch/cpp-utils/timer.h"
#include "nch/sdl-utils/input.h"
#include "nch/sdl-utils/input.h"


/* Emscripten app support */
#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif

using namespace nch;


bool MainLoopDriver::mldExists = false;
bool MainLoopDriver::running = true;
int MainLoopDriver::targetFPS = -1, MainLoopDriver::targetTPS = -1;
uint64_t MainLoopDriver::numTicksPassedTotal = 0;
uint64_t MainLoopDriver::nsPerFrame;
int MainLoopDriver::fps, MainLoopDriver::tps;
uint64_t MainLoopDriver::nextFrameNS;
uint64_t MainLoopDriver::numTicksPassedThisSec;
SDL_Renderer* MainLoopDriver::rend;
bool MainLoopDriver::loggingPerformance = false;
std::string MainLoopDriver::performanceInfo = "???null???";
int MainLoopDriver::currentFPS = 0, MainLoopDriver::currentTPS = 0;
std::mutex MainLoopDriver::mtx;
int MainLoopDriver::currentNumTicksLeft = 0;
uint64_t MainLoopDriver::lastTickNS = 0;
bool MainLoopDriver::manualTicker = false;
uint64_t MainLoopDriver::numTicksPassed = 0;
uint64_t MainLoopDriver::nextTickNS = 0;

void (*MainLoopDriver::tickFunc)() = nullptr;
void (*MainLoopDriver::altDrawFunc)() = nullptr;
void (*MainLoopDriver::drawFunc)(SDL_Renderer*) = nullptr;
void (*MainLoopDriver::eventFunc)(SDL_Event&) = nullptr;

/**/

MainLoopDriver::MainLoopDriver(SDL_Renderer* rend, void (*tickFunc)(), uint64_t targetTPS, void (*drawFunc)(SDL_Renderer*), uint64_t targetFPS, void (*eventFunc)(SDL_Event&)) {
	start(rend, tickFunc, targetTPS, drawFunc, nullptr, targetFPS, eventFunc);
}
MainLoopDriver::MainLoopDriver(SDL_Renderer* rend, void (*tickFunc)(), uint64_t targetTPS, void (*drawFunc)(SDL_Renderer*), uint64_t targetFPS)
:MainLoopDriver(rend, tickFunc, targetTPS, drawFunc, targetFPS, nullptr){}

MainLoopDriver::MainLoopDriver(void (*tickFunc)(), uint64_t targetTPS, void (*altDrawFunc)(), uint64_t targetFPS, void (*eventFunc)(SDL_Event&)) {
	start(nullptr, tickFunc, targetTPS, nullptr, altDrawFunc, targetFPS, eventFunc);
}
MainLoopDriver::MainLoopDriver(void (*tickFunc)(), uint64_t targetTPS, void (*altDrawFunc)(), uint64_t targetFPS)
:MainLoopDriver(tickFunc, targetTPS, altDrawFunc, targetFPS, nullptr){}

/**/

uint64_t MainLoopDriver::getTargetNSPT() {
	return 1000000000/(uint64_t)targetTPS;
}
uint64_t MainLoopDriver::getTargetNSPF() {
	return 1000000000/(uint64_t)targetFPS;
}
int MainLoopDriver::getCurrentTPS() { return currentTPS; }
int MainLoopDriver::getCurrentFPS() { return currentFPS; }
std::string MainLoopDriver::getPerformanceInfo()
{
	std::string res = Log::getFormattedString("(FPS, TPS)=(%d/%d, %d/%d)", currentFPS, targetFPS, currentTPS, targetTPS);
	return res;
}
uint64_t MainLoopDriver::getNumTicksPassedTotal() { return numTicksPassedTotal; }

bool MainLoopDriver::hasQuit() {
	return !running;
}

void MainLoopDriver::quit() {
	running = false;
}

void MainLoopDriver::start(SDL_Renderer* rend, void (*tickFunc)(), uint64_t targetTPS, void (*drawFunc)(SDL_Renderer*), void (*altDrawFunc)(), uint64_t targetFPS, void (*eventFunc)(SDL_Event&))
{
	/* Track whether main loop driver exists */
	{
		if(mldExists) {
			Log::warn(__PRETTY_FUNCTION__, "A MainLoopDriver has already been created");
			return;
		}
		mldExists = true;
	}

	/* Set MLD members */
	{
		MainLoopDriver::rend = rend;
		//Tick
		MainLoopDriver::tickFunc = tickFunc;
		MainLoopDriver::targetTPS = targetTPS;
		tps = 0;
		numTicksPassedThisSec = 0;
		//Draw
		MainLoopDriver::drawFunc = drawFunc;
		MainLoopDriver::altDrawFunc = altDrawFunc;
		MainLoopDriver::targetFPS = targetFPS;
		fps = 0;
		nsPerFrame = 1000000000/(uint64_t)targetFPS;
		nextFrameNS = 0;
		//Events
		MainLoopDriver::eventFunc = eventFunc;
	}

	/* Run main loop */
	{
		#ifdef EMSCRIPTEN
			Log::log("Using an Emscripten main loop for this MainLoopDriver.");
			manualTicker = true;
			emscripten_set_main_loop(mainLoop, 0, 1);
		#else
			std::thread tickerThread(MainLoopDriver::ticker);
			while(running) mainLoop();
			tickerThread.detach();
		#endif
	}

	//Quit once main loop has finished.
	SDL_Quit();
}

void MainLoopDriver::mainLoop(void)
{
	#ifdef EMSCRIPTEN
		for(int i = 0; i<10; i++) {
			ticker();
		}
		if(!running) {
			emscripten_cancel_main_loop();
		}
	#endif

	//Tick as many times as currently needed by the program (may be 0 or more)
	while(currentNumTicksLeft>0) {
		const std::lock_guard<std::mutex> lock(mtx);
		numTicksPassedThisSec++;
		currentNumTicksLeft--;

		lastTickNS = Timer::getCurrentTimeNS();
		Input::tick();
		tickFunc();
		tps++;
		numTicksPassedTotal++;
	}

	//Draw once if we should (never draw multiple times at once)
	if(Timer::getCurrentTimeNS()>=nextFrameNS) {
		nextFrameNS = Timer::getCurrentTimeNS()+nsPerFrame;
		if(drawFunc!=nullptr) drawFunc(rend);
		if(altDrawFunc!=nullptr) altDrawFunc();
		fps++;
	}
	
	//Events
	events();
	
	//Run this block every second.
	if(numTicksPassedThisSec>=targetTPS) {
		numTicksPassedThisSec -= targetTPS;
		currentTPS = tps;
		currentFPS = fps;
		tps = 0;
		fps = 0;
	}

	//Sleep for ~1ms so CPU core usage not overused.
	//This should not affect tickrate as 'currentNumTicksLeft' allows for ticks to "catch up"
	Timer::sleep(1);
}


void MainLoopDriver::ticker()
{
	//Tick loop
	uint64_t nsPerTick = 1000000000/(uint64_t)targetTPS;
	bool tickOnce = true;

	while(!manualTicker || tickOnce) {
		//Get current time
		uint64_t nowNS = Timer::getCurrentTimeNS();

		//Fix "speeding up" behavior if more than a second has passed from last tick to this tick.
		if(nowNS-lastTickNS>1000000000) {
			const std::lock_guard<std::mutex> lock(mtx);
			numTicksPassed = nowNS/nsPerTick-1;
			currentNumTicksLeft = 0;
			nextTickNS = nsPerTick*numTicksPassed;
			lastTickNS = nsPerTick*(numTicksPassed-1);
		}

		//If the current time exceeds the time of the next tick, schedule a new tick using 'currentNumTicksLeft'.
		while(nowNS>=nextTickNS) {
			const std::lock_guard<std::mutex> lock(mtx);
			
			numTicksPassed++;
			currentNumTicksLeft++;
			nextTickNS = nsPerTick*numTicksPassed;
		}

		tickOnce = false;
	}
}

void MainLoopDriver::events() {
	SDL_Event e;
	while( SDL_PollEvent(&e)!=0 ) {
		Input::allEvents(e);

		switch(e.type) {
			case SDL_QUIT: {
				running = false;
			} break;
			
			case SDL_KEYDOWN:				case SDL_KEYUP:
			case SDL_MOUSEBUTTONDOWN:		case SDL_MOUSEBUTTONUP:
			case SDL_MOUSEWHEEL:
			case SDL_JOYBUTTONDOWN:			case SDL_JOYBUTTONUP:
			case SDL_CONTROLLERBUTTONDOWN:	case SDL_CONTROLLERBUTTONUP:
			case SDL_JOYHATMOTION:
			{
				Input::inputEvents(e);
			} break;
		}

		if(eventFunc!=nullptr) eventFunc(e);
	}
}
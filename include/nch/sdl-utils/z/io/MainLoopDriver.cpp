#include "MainLoopDriver.h"
#include <SDL2/SDL_timer.h>
#include <thread>
#include "nch/cpp-utils/log.h"
#include "nch/cpp-utils/timer.h"
#include "nch/sdl-utils/input.h"
#include "nch/sdl-utils/input.h"

using namespace nch;

bool MainLoopDriver::mldExists = false;
bool MainLoopDriver::running = true;
int MainLoopDriver::targetFPS = -1, MainLoopDriver::targetTPS = -1;
uint64_t MainLoopDriver::numTicksPassedTotal = 0;

bool MainLoopDriver::loggingPerformance = false;
std::string MainLoopDriver::performanceInfo = "???null???";
int MainLoopDriver::currentFPS = 0, MainLoopDriver::currentTPS = 0;

std::mutex MainLoopDriver::mtx;
int MainLoopDriver::currentNumTicksLeft = 0;
uint64_t MainLoopDriver::lastTickNS = 0;

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

void MainLoopDriver::quit() {
	running = false;
}

void MainLoopDriver::start(SDL_Renderer* rend, void (*tickFunc)(), uint64_t targetTPS, void (*drawFunc)(SDL_Renderer*), void (*altDrawFunc)(), uint64_t targetFPS, void (*eventFunc)(SDL_Event&))
{
	if(mldExists) {
		Log::warn(__PRETTY_FUNCTION__, "A MainLoopDriver has already been created");
		return;
	}
	mldExists = true;

	//Set target FPS + TPS
	MainLoopDriver::targetTPS = targetTPS;
	MainLoopDriver::targetFPS = targetFPS;

	//Set draw, tick, and ticker callbacks
	MainLoopDriver::tickFunc = tickFunc;
	MainLoopDriver::drawFunc = drawFunc;
	MainLoopDriver::altDrawFunc = altDrawFunc;
	MainLoopDriver::eventFunc = eventFunc;
	std::thread tickerThread(MainLoopDriver::ticker);

	//On main thread, run game loop as long as needed
	uint64_t nsPerFrame = 1000000000/(uint64_t)targetFPS;
	int fps = 0, tps = 0;
	uint64_t nextFrameNS = 0;
	uint64_t numTicksPassedThisSec = 0;
	while(running) {
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
		Timer::sleep(1);
	}

	//Upon running==false, cleanup and quit.
	tickerThread.detach();
	SDL_Quit();
}


void MainLoopDriver::ticker()
{
	//Tick loop
	uint64_t nsPerTick = 1000000000/(uint64_t)targetTPS;
	uint64_t numTicksPassed = 0;	//Number of ticks that should have passed according to time since launch
	uint64_t nextTickNS = 0;		//Time of the next tick
	while(true) {
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

		Timer::sleep(1);
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
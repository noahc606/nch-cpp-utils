#include "MainLoopDriver.h"
#include <nch/sdl-utils/Input.h>
#include <nch/sdl-utils/Timer.h>
#include <SDL2/SDL_timer.h>

NCH_MainLoopDriver::NCH_MainLoopDriver(SDL_Renderer* rend, void (*tickFunc)(), uint64_t targetTPS, void (*drawFunc)(SDL_Renderer*), uint64_t targetFPS)
{
	NCH_MainLoopDriver::tickFunc = tickFunc;
	NCH_MainLoopDriver::drawFunc = drawFunc;
	
	int tps = 0;
	int fps = 0;
	uint64_t tickNextNS = 0;
	uint64_t frameNextNS = 0;

	//Run while game is running, every millisecond.
	while(running) {
		uint64_t nsPerTick = 1000000000/targetTPS;
		uint64_t nsPerFrame = 1000000000/targetFPS;

		//If the game is ready to tick
		if(NCH_Timer::getCurrentTimeNS()>=tickNextNS) {
			//Update when the next tick should happen
			tickNextNS = NCH_Timer::getCurrentTimeNS()+nsPerTick;

			//Perform the tick, calculating how much time it takes.
			uint64_t tickT0 = NCH_Timer::getCurrentTimeNS();
			tickFunc();
			NCH_Input::tick();
			tps++;
			uint64_t tickT1 = NCH_Timer::getCurrentTimeNS();
			uint64_t tickDeltaNS = tickT1-tickT0;

			/* Store how long the last 'targetTPS' ticks have taken within 'tickTimesNS' */
			//If we have 'targetTPS' elements, erase the first (and oldest) one.
			while(tickTimesNS.size()>=targetTPS) {
				tickTimesNS.erase(tickTimesNS.begin());
			}
			//Add the latest time.
			tickTimesNS.push_back(tickDeltaNS);
		}

		//If the game is ready to draw (new frame)
		if(NCH_Timer::getCurrentTimeNS()>=frameNextNS) {
			//Update when the next frame should be drawn
			frameNextNS = NCH_Timer::getCurrentTimeNS()+nsPerFrame;

			//Perform the draw, calculating how much time it takes.
			uint64_t frameT0 = NCH_Timer::getCurrentTimeNS();
			drawFunc(rend); fps++;
			uint64_t frameT1 = NCH_Timer::getCurrentTimeNS();
			uint64_t frameDeltaNS = frameT1-frameT0;

			/* Store how long the last 'targetFPS' frames have taken within 'frameTimesNS */
			//If we have >= 'targetFPS' elements, erase the oldest ones.
			while(frameTimesNS.size()>=targetFPS) {
				frameTimesNS.erase(frameTimesNS.begin());
			}
			//Add the latest time.
			frameTimesNS.push_back(frameDeltaNS);
		}

		if(getAvgNSPT()>nsPerTick) {
			if(targetFPS>5) targetFPS--;
		} else {
			if(targetFPS<hardMaxFPS) targetFPS++;
		}

		//Run this block every second.
		if( NCH_Timer::getTicks64()>=secLast ) {
			secLast = NCH_Timer::getTicks64()+1000;
			currentTPS = tps;
			currentFPS = fps;

            if(loggingPerformance) {
                printf("(FPS/target, TPS/target)=(%d/%d, %d/%d). NSPF=%d\n", currentFPS, targetFPS, currentTPS, targetTPS, getAvgNSPF());
            }

			tps = 0;
			fps = 0;
		}

		events();
	}
}

uint64_t NCH_MainLoopDriver::getAvgNSPT()
{
	uint64_t res = 0;
	for(int i = 0; i<tickTimesNS.size(); i++) {
		res += tickTimesNS[i];
	}
	return res/tickTimesNS.size();
}
uint64_t NCH_MainLoopDriver::getAvgNSPF()
{
	uint64_t res = 0;
	for(int i = 0; i<frameTimesNS.size(); i++) {
		res += frameTimesNS[i];
	}
	return res/frameTimesNS.size();
}

void NCH_MainLoopDriver::events() {
	SDL_Event e;
	while( SDL_PollEvent(&e)!=0 ) {
		NCH_Input::anyEvents(e);

		switch(e.type) {
			case SDL_QUIT: {
				running = false;
				SDL_Quit();
			} break;
			
			case SDL_KEYDOWN:
			case SDL_KEYUP:
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP: {
				NCH_Input::events(e);
			} break;
		}
	}	
}
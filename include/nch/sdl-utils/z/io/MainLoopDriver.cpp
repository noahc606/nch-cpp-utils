#include "MainLoopDriver.h"
#include <assert.h>
#include <chrono>
#include <thread>
#include "nch/cpp-utils/color.h"
#include "nch/cpp-utils/log.h"
#include "nch/cpp-utils/timer.h"
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
std::vector<double> MainLoopDriver::frameTimes, MainLoopDriver::tickTimes;
std::map<std::string, nch::Color> MainLoopDriver::bmLabelColors;
std::map<std::string, std::vector<double>> MainLoopDriver::bmFrameTimes, MainLoopDriver::bmTickTimes;
std::vector<SDL_Vertex> MainLoopDriver::bmVerts;
std::vector<int> MainLoopDriver::bmInds;
std::vector<int> MainLoopDriver::bmStackTops;

std::mutex MainLoopDriver::mtx;
int MainLoopDriver::currentNumTicksLeft = 0;
uint64_t MainLoopDriver::lastTickNS = 0;
bool MainLoopDriver::manualTicker = false;
uint64_t MainLoopDriver::numTicksPassed = 0;
uint64_t MainLoopDriver::nextTickNS = 0;

void (*MainLoopDriver::tickFunc)() = nullptr;
void (*MainLoopDriver::altDrawFunc)() = nullptr;
void (*MainLoopDriver::eventFunc)(SDL_Event&) = nullptr;

/**/

MainLoopDriver::MainLoopDriver(void (*tickFunc)(), uint64_t targetTPS, void (*altDrawFunc)(), uint64_t targetFPS, void (*eventFunc)(SDL_Event&)) {
	start(nullptr, tickFunc, targetTPS, altDrawFunc, targetFPS, eventFunc);
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
int MainLoopDriver::getTargetFPS() { return targetFPS; }
void MainLoopDriver::setTargetFPS(int newTargetFPS) {
	if(newTargetFPS<10) newTargetFPS = 10;
	if(newTargetFPS>1000) newTargetFPS = 1000;
	if(newTargetFPS==targetFPS) return;

	targetFPS = newTargetFPS;
	nsPerFrame = 1000000000/(uint64_t)targetFPS;
	//Rebase the pending frame deadline: one scheduled under an old longer period would otherwise
	//delay the first frame after raising the cap.
	uint64_t nowNS = Timer::getCurrentTimeNS();
	if(nextFrameNS>nowNS+nsPerFrame) {
		nextFrameNS = nowNS+nsPerFrame;
	}
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

void MainLoopDriver::drawPerformanceBenchmark(GLSDL_Renderer* sdlRend, int bmHeight, int windowWidth, int windowHeight, void* ttfFont) {
	SDL_BlendMode oldBlendMode;
	GLSDL_GetRenderDrawBlendMode(sdlRend, &oldBlendMode);
	GLSDL_SetRenderDrawBlendMode(sdlRend, SDL_BLENDMODE_BLEND);

	//One graph per column, on one baseline: the raw TOTAL goes down first and the instrumented ops
	//stack over it. The colored stack is the known footprint, so whatever raw line still shows above it
	//is the part of that frame/tick nothing accounts for yet — which is the thing worth looking at.
	int bottom = windowHeight;
	int tickX = windowWidth-targetTPS;
	int frameX = 0;

	bmVerts.clear();
	bmInds.clear();

	drawPane(frameX, bottom, targetFPS, bmHeight, Color(0, 255, 255));
	drawPane(tickX,  bottom, targetTPS, bmHeight, Color(255, 0, 255));

	//Frame times
	double idealMSPF = 1000.0/targetFPS;
	drawRawRow(frameTimes, targetFPS, frameX, bottom, bmHeight, idealMSPF, Color(255, 0, 0));
	drawBreakdownRow(bmFrameTimes, targetFPS, frameX, bottom, bmHeight, idealMSPF);
	//Tick times
	double idealMSPT = 1000.0/targetTPS;
	drawRawRow(tickTimes, targetTPS, tickX, bottom, bmHeight, idealMSPT, Color(0, 255, 0));
	drawBreakdownRow(bmTickTimes, targetTPS, tickX, bottom, bmHeight, idealMSPT);

	flushBars(sdlRend);

	GLSDL_SetRenderDrawBlendMode(sdlRend, oldBlendMode);
}
void MainLoopDriver::pushBar(int x0, int top, int w, int h, const nch::Color& color, uint8_t alpha) {
	if(w<=0 || h<=0) return;

	SDL_Color c = {color.r, color.g, color.b, alpha};
	float x1 = (float)x0, x2 = (float)(x0+w);
	float y1 = (float)top, y2 = (float)(top+h);
	int base = (int)bmVerts.size();

	SDL_Vertex v;
	v.color = c;
	v.tex_coord = {0, 0};
	v.position = {x1, y1}; bmVerts.push_back(v);
	v.position = {x2, y1}; bmVerts.push_back(v);
	v.position = {x2, y2}; bmVerts.push_back(v);
	v.position = {x1, y2}; bmVerts.push_back(v);

	bmInds.push_back(base+0); bmInds.push_back(base+1); bmInds.push_back(base+2);
	bmInds.push_back(base+2); bmInds.push_back(base+3); bmInds.push_back(base+0);
}
void MainLoopDriver::flushBars(GLSDL_Renderer* sdlRend) {
	if(bmInds.empty()) return;
	//Untextured, so the per-vertex colors carry both graphs' palettes through the one call.
	GLSDL_RenderGeometry(sdlRend, nullptr, bmVerts.data(), (int)bmVerts.size(), bmInds.data(), (int)bmInds.size());
}
void MainLoopDriver::drawPane(int x0, int bottom, int w, int bmHeight, const nch::Color& color) {
	pushBar(x0, bottom-bmHeight, w, bmHeight, color, 191);
}
void MainLoopDriver::drawRawRow(const std::vector<double>& times, int maxSamples,
	int x0, int bottom, int bmHeight, double idealMS, const nch::Color& color)
{
	int numSamples = (int)times.size()<maxSamples ? (int)times.size() : maxSamples;
	for(int i = 0; i<numSamples; i++) {
		int lineSize = (int)(bmHeight*times[i]/idealMS)+1;
		pushBar(x0+i, bottom-lineSize, 1, lineSize, color, 255);
	}
}
void MainLoopDriver::drawBreakdownRow(const std::map<std::string, std::vector<double>>& bmTimes,
	int maxSamples, int x0, int bottom, int bmHeight, double idealMS)
{
	//Label-outer over each label's own samples, against one running stack top per index. Sample-outer
	//visited every label at every index, so at a high FPS cap most of the work was skipping labels with
	//no sample there yet, and each label's color was looked up again for every sample.
	bmStackTops.assign(maxSamples, bottom);

	for(const auto& bm : bmTimes) {
		Color lineColor(255, 255, 255);
		auto colItr = bmLabelColors.find(bm.first);
		if(colItr!=bmLabelColors.end()) lineColor = colItr->second;

		int numSamples = (int)bm.second.size()<maxSamples ? (int)bm.second.size() : maxSamples;
		for(int i = 0; i<numSamples; i++) {
			int lineSize = (int)(bmHeight*bm.second[i]/idealMS)+1;
			bmStackTops[i] -= lineSize;
			pushBar(x0+i, bmStackTops[i], 1, lineSize, lineColor, 255);
		}
	}
}
void MainLoopDriver::performanceBenchmarkDrawOp(Timer& timer, const nch::Color& color) {
	const std::string lbl = timer.getDesc();
	auto vecItr = bmFrameTimes.find(lbl);
	if(vecItr==bmFrameTimes.end()) {
		std::vector<double> times;
		bmFrameTimes.insert({lbl, times});
		vecItr = bmFrameTimes.find(lbl);
	}
	assert(vecItr!=bmFrameTimes.end());

	auto colItr = bmLabelColors.find(lbl);
	if(colItr==bmLabelColors.end()) {
		bmLabelColors.insert({lbl, color});
	}
	
	//At start of draw: Clear, populate with empty
	if(fps==0) {
		vecItr->second.clear();
	}
	vecItr->second.push_back(timer.getElapsedTimeMS());
}
void MainLoopDriver::performanceBenchmarkTickOp(Timer& timer, const nch::Color& color) {
	performanceBenchmarkTickOp(timer.getDesc(), timer.getElapsedTimeMS(), color);
}
void MainLoopDriver::performanceBenchmarkTickOp(const std::string& lbl, double ms, const nch::Color& color) {
	auto vecItr = bmTickTimes.find(lbl);
	if(vecItr==bmTickTimes.end()) {
		std::vector<double> times;
		bmTickTimes.insert({lbl, times});
		vecItr = bmTickTimes.find(lbl);
	}
	assert(vecItr!=bmTickTimes.end());

	auto colItr = bmLabelColors.find(lbl);
	if(colItr==bmLabelColors.end()) {
		bmLabelColors.insert({lbl, color});
	}

	//At start of tick cycle: Clear, populate with empty
	if(tps==0) {
		vecItr->second.clear();
	}
	vecItr->second.push_back(ms);
}

void MainLoopDriver::quit() {
	running = false;
}
void MainLoopDriver::start(SDL_Renderer* rend, void (*tickFunc)(), uint64_t targetTPS, void (*altDrawFunc)(), uint64_t targetFPS, void (*eventFunc)(SDL_Event&)) {
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
		tickTimes.clear(); tickTimes.reserve(targetTPS);
		for(int i = 0; i<targetTPS; i++) {
			tickTimes.push_back(0);
		}
		//Draw
		MainLoopDriver::altDrawFunc = altDrawFunc;
		MainLoopDriver::targetFPS = targetFPS;
		fps = 0;
		nsPerFrame = 1000000000/(uint64_t)targetFPS;
		nextFrameNS = 0;
		frameTimes.clear(); frameTimes.reserve(targetFPS);
		for(int i = 0; i<targetFPS; i++) {
			frameTimes.push_back(0);
		}
		//Events
		MainLoopDriver::eventFunc = eventFunc;
		//Input init
		Input::tick();
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
		Timer tim("tick");
		
		const std::lock_guard<std::mutex> lock(mtx);
		numTicksPassedThisSec++;
		currentNumTicksLeft--;
		lastTickNS = Timer::getCurrentTimeNS();
		
		Input::tick();
		tickFunc();
		if(tps==0) tickTimes.clear();

		tps++; numTicksPassedTotal++;
		tickTimes.push_back(tim.getElapsedTimeMS());
	}

	//Draw once if we should (never draw multiple times at once)
	if(Timer::getCurrentTimeNS()>=nextFrameNS) {
		Timer tim("draw");

		//Accumulate the deadline instead of rescheduling from 'now'. Scheduling from now folded every
		//frame's lateness (sleep overshoot, a tick landing just before the deadline) permanently into
		//the schedule, settling the achieved rate at 1/(nsPerFrame+overshoot) rather than the target.
		//Accumulating lets a late frame be followed by a short one, keeping the average exact.
		uint64_t nowNS = Timer::getCurrentTimeNS();
		nextFrameNS += nsPerFrame;
		//A whole period behind (hitch, or a cap this machine can't sustain): resync rather than paying
		//the debt back as a burst of back-to-back frames.
		if(nextFrameNS<nowNS) {
			nextFrameNS = nowNS+nsPerFrame;
		}

		if(altDrawFunc!=nullptr) altDrawFunc();
		if(fps==0) frameTimes.clear();

		fps++;
		frameTimes.push_back(tim.getElapsedTimeMS());
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
		bmFrameTimes.clear();
		bmTickTimes.clear();
	}

	#ifdef EMSCRIPTEN
		//Browser rAF paces the loop; just yield briefly as before.
		Timer::sleep(1);
	#else
		//Sleep until the next deadline (tick or frame) so CPU isn't overused. The former fixed 1ms
		//nap taxed every frame, capping achievable FPS at ~1000/(drawTimeMS+1). If a deadline is
		//already due, don't sleep at all. Tick catch-up via 'currentNumTicksLeft' is unaffected.
		uint64_t soonestNS;
		{
			const std::lock_guard<std::mutex> lock(mtx);
			soonestNS = nextTickNS<nextFrameNS ? nextTickNS : nextFrameNS;
		}
		uint64_t nowNS = Timer::getCurrentTimeNS();
		if(soonestNS>nowNS) {
			std::this_thread::sleep_for(std::chrono::nanoseconds(soonestNS-nowNS));
		}
	#endif
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
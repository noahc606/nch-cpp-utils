#pragma once
#include <GLSDL/GLSDL.h>
#include <SDL2/SDL_events.h>
#include <map>
#include <mutex>
#include <string>
#include <vector>
#include "nch/cpp-utils/color.h"
#include "nch/cpp-utils/timer.h"
#include "nch/sdl-utils/text.h"

namespace nch { class MainLoopDriver {
public:
    MainLoopDriver(void (*tickFunc)(), uint64_t targetTPS, void (*altDrawFunc)(), uint64_t targetFPS, void (*eventFunc)(SDL_Event&));
    MainLoopDriver(void (*tickFunc)(), uint64_t targetTPS, void (*altDrawFunc)(), uint64_t targetFPS);
    
    static uint64_t getTargetNSPT();
    static uint64_t getTargetNSPF();
    static int getTargetFPS();
    //Change the FPS cap at runtime (clamped to [10, 1000]); takes effect on the next frame.
    //Call from the tick/draw callbacks (the main-loop thread) — not from other threads.
    static void setTargetFPS(int newTargetFPS);
    static int getCurrentTPS();
    static int getCurrentFPS();
    static std::string getPerformanceInfo();
    static uint64_t getNumTicksPassedTotal();
    static bool hasQuit();
    
    /**
     * @brief Draw the frame graph (bottom-left) and the tick graph (bottom-right). Call between GLSDL_GL_SaveState()
     * and GLSDL_GL_RestoreState().
     * @param font Font to bake each graph's legend from, or nullptr for no legends. The labels are baked once and
     * rebaked only when a different font is passed.
     * @param textScale Scale the legend text is drawn at.
     */
    static void drawPerformanceBenchmark(GLSDL_Renderer* sdlRend, int bmHeight, int windowWidth, int windowHeight, TTF_Font* font = nullptr, double textScale = 1.0);
    static void performanceBenchmarkDrawOp(nch::Timer& timer, const nch::Color& color);
    static void performanceBenchmarkTickOp(nch::Timer& timer, const nch::Color& color);
    //Same, for a contribution that isn't one Timer's lifetime — work spread over several calls in a tick
    //has to be summed by the caller and reported once, since a label's samples are indexed by tick.
    static void performanceBenchmarkTickOp(const std::string& label, double ms, const nch::Color& color);
    static void quit();
private:
    void start(SDL_Renderer* rend, void (*tickFunc)(), uint64_t targetTPS, void (*altDrawFunc)(), uint64_t targetFPS, void (*eventFunc)(SDL_Event&));
    static void mainLoop();

    //Everything the benchmark draws is one axis-aligned bar, batched by these into a single draw call.
    //A GLSDL draw call generates and deletes a VAO and two VBOs, so a call per 1px column cost that
    //once per sample per label — and since a second's samples pile up until the reset, the profiler
    //grew into the most expensive thing in the frame by the end of every second.
    static void pushBar(int x0, int top, int w, int h, const nch::Color& color, uint8_t alpha);
    static void flushBars(GLSDL_Renderer* sdlRend);

    //One background panel: <w> wide and <bmHeight> tall, sitting on <bottom>.
    static void drawPane(int x0, int bottom, int w, int bmHeight, const nch::Color& color);
    //The raw totals: one 1px column per sample, growing up from <bottom>. Drawn BEHIND the breakdown,
    //so what stays visible of it is the unaccounted-for part of each sample.
    static void drawRawRow(const std::vector<double>& times, int maxSamples,
        int x0, int bottom, int bmHeight, double idealMS, const nch::Color& color);
    //The instrumented ops for those same samples, stacked in label order, growing up from <bottom>.
    static void drawBreakdownRow(const std::map<std::string, std::vector<double>>& bmTimes,
        int maxSamples, int x0, int bottom, int bmHeight, double idealMS);

    //One graph's legend: a name for every label it has ever stacked. Outlives the samples, which are cleared
    //every second, so a label that hasn't reported yet this second doesn't blink out of it.
    struct Legend {
        std::map<std::string, nch::Text> labelTexts;
        nch::Text rawText; //The raw row's own entry: the time nothing accounts for.
    };
    //A legend name waiting for the bar batch to be flushed, so it draws over it.
    struct LegendLine {
        const nch::Text* txt = nullptr;
        int x = 0;
        int y = 0;
    };
    //Rebuild the legends from scratch when <font> or <sdlRend> changed since the last call (a resource reload
    //frees the old font), and make sure both exist.
    static void prepLegends(GLSDL_Renderer* sdlRend, TTF_Font* font);
    //Free both legends and every texture baked for them.
    static void releaseLegends();
    //Bake <txt> as <label> the first time it's seen, and keep it drawing at <textScale>.
    static void prepLegendText(nch::Text& txt, const std::string& label, double textScale);
    //Name every label of <bmTimes>, bottom-up in the order the breakdown stacks them, under the raw row's own
    //entry. <anchorX> is the legend's left edge, or its right edge when <rightAligned>.
    static void layoutLegend(Legend& legend, const std::map<std::string, std::vector<double>>& bmTimes,
        const nch::Color& rawColor, const nch::Color& paneColor, int anchorX, int bottom, bool rightAligned, double textScale);
    //One entry sitting on <bottom>: a <color> swatch pushed into the bar batch, framed in <paneColor> because a
    //dark bar color only reads against the pane it's drawn on, with <txt> queued beside it. Returns its top.
    static int pushLegendLine(const nch::Text& txt, const nch::Color& color, const nch::Color& paneColor,
        int anchorX, int bottom, bool rightAligned);

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
    static std::map<std::string, std::vector<double>> bmFrameTimes, bmTickTimes;
    static std::map<std::string, nch::Color> bmLabelColors;
    //Kept between frames so the batch's capacity is only ever paid for once.
    static std::vector<SDL_Vertex> bmVerts;
    static std::vector<int> bmInds;
    //One running stack top per sample index, so a breakdown can be walked label-outer.
    static std::vector<int> bmStackTops;
    //Heap-held and freed by releaseLegends() rather than static objects: freeing a texture needs a GL save state,
    //and exit-time destructors run after the context is already gone.
    static Legend* bmFrameLegend;
    static Legend* bmTickLegend;
    static std::vector<LegendLine> bmLegendLines;
    static GLSDL_Renderer* bmRend;
    static TTF_Font* bmLegendFont;
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
    static void (*eventFunc)(SDL_Event&);
};}
#include <iostream>
#include <nch/cpp-utils/arraylist.h>
#include <nch/cpp-utils/fs-utils.h>
#include <nch/cpp-utils/color.h>
#include <nch/cpp-utils/log.h>
#include <nch/cpp-utils/string-utils.h>
#include <nch/cpp-utils/timer.h>
#include <nch/sdl-utils/z/debug/SDLEventDebugger.h>
#include <nch/sdl-utils/text.h>
#include <nch/sdl-utils/texture-utils.h>
#include <nch/sdl-utils/input.h>
#include <nch/sdl-utils/main-loop-driver.h>
#include <SDL2/SDL.h>
#include <sstream>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
using namespace nch;

bool doBackground = true;
bool firstDraw = true;
int64_t tickTimer = -10;
int64_t drawTimer = -10;
GLSDL_Texture* tex = nullptr;
GLSDL_Window* win = nullptr;
GLSDL_Renderer* rend = nullptr;
uint32_t winPixFormat = 0;
ArrayList<Text> dbgScreen;
TTF_Font* dbgFont = nullptr;
std::string basePath = "";

int getWidth() {
    int width = 0;
    SDL_GetWindowSize(win->toSDL_Window(), &width, NULL);
    return width;
}
int getHeight() {
    int height = 0;
    SDL_GetWindowSize(win->toSDL_Window(), NULL, &height);
    return height;
}

void drawInfo()
{
    int width = getWidth();
    int height = getHeight();


    {
        dbgScreen[0].setText(StringUtils::cat("CurrentTimeNS=", Timer::getCurrentTimeNS()));
        dbgScreen[1].setText(MainLoopDriver::getPerformanceInfo());
        dbgScreen[2].setText(StringUtils::cat("Window dimensions (W x H) = ", width, " x ", height, "."));
        dbgScreen[3].setText(StringUtils::cat("Last Known Input Event ID: ", Input::getLastKnownSDLEventID()));
        dbgScreen[4].setText(Input::getLastKnownSDLEventDesc());
    }
    

    int secs = -1;
    int pct = -1;
    std::string powerState = "";
    SDL_PowerState sps = SDL_GetPowerInfo(&secs, &pct);
    switch(sps) {
        case SDL_PowerState::SDL_POWERSTATE_CHARGED: { powerState = "Charged"; } break;
        case SDL_PowerState::SDL_POWERSTATE_CHARGING: { powerState = "Charging"; } break;
        case SDL_PowerState::SDL_POWERSTATE_NO_BATTERY: { powerState = "No Battery"; } break;
        case SDL_PowerState::SDL_POWERSTATE_ON_BATTERY: { powerState = "On Battery"; } break;
        case SDL_PowerState::SDL_POWERSTATE_UNKNOWN: { powerState = "Unknown"; } break;
    }    
    dbgScreen[5].setText(StringUtils::cat("Battery: ", pct, "% (~", secs, "s left). Power State: ", powerState));

    double scale = 0.125*1;
    for(int i = 0; i<dbgScreen.size(); i++) {
        dbgScreen.at(i).setScale(scale);
    }
    

    for(int i = 0; i<dbgScreen.size(); i++) {
        dbgScreen.at(i).draw(width/2-dbgScreen.at(i).getWidth()/2, height/3+dbgScreen.at(0).getHeight()*i);
    }
}

void draw()
{
    GLSDL_GL_SaveState(rend);

    //Only on first draw, create a clear texture that is 640x480.
    if(firstDraw) {
        if(doBackground) {
            tex = GLSDL_CreateTexture(rend, winPixFormat, SDL_TEXTUREACCESS_TARGET, 640, 480);
            nch::TexUtils::clearTexture(rend, tex);
        }
        firstDraw = false;
    }

    //Reset screen
    GLSDL_RenderClear(rend);
    GLSDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
    GLSDL_RenderFillRect(rend, NULL);

    if(doBackground) {
        //Update parts of texture depending on timer
        GLSDL_SetRenderTarget(rend, tex);
        if(drawTimer>=0 && drawTimer<=480) {
            nch::Color c(255, 255, 255);
            int iy = drawTimer;
            for(int ix = 0; ix<640; ix++) {
                int i = iy*640+ix;
                c.setFromHSV(std::abs(ix+i/1000)%360, 100*iy/480, 100-(100*iy/480));
                GLSDL_SetRenderDrawColor(rend, c.r, c.g, c.b, 255);
                GLSDL_RenderDrawPoint(rend, ix, iy);
            }
        }
        GLSDL_SetRenderTarget(rend, NULL);

        //Draw texture and give it a color depending on the current tick timer
        nch::Color c2(255, 255, 255);
        c2.setFromHSV( std::abs(tickTimer%360), std::abs(tickTimer/3)%100, 100 );
        GLSDL_SetTextureColorMod(tex, c2.r, c2.g, c2.b);
        GLSDL_RenderCopy(rend, tex, NULL, NULL);
    }

    drawInfo();

    nch::MainLoopDriver::drawPerformanceBenchmark(rend, 100, getWidth(), getHeight());

    //Render all present objects
    GLSDL_RenderPresent(rend);

    //Increment draw timer
    drawTimer++;

    GLSDL_GL_RestoreState(rend);
}

void tick()
{
    //Increment tick timer
    tickTimer++;
}

int main(int argc, char **argv)
{
    /* Say hello */
    Log::log("Hello world");

    /* Init SDL, create window and renderer */
    {
        //SDL
        if(GLSDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK)<0) {
            printf("SDL_Init Error: %s\n", SDL_GetError());
        }
        //Window
        win = GLSDL_CreateWindow("NCH-CPP-Utils Example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_RESIZABLE);
        if(win==NULL) {
            printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        }
        SDL_RaiseWindow(win->toSDL_Window());
        winPixFormat = SDL_GetWindowPixelFormat(win->toSDL_Window());
        //Renderer
        rend = GLSDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
        if(rend==NULL) {
            printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
        }
        #ifdef EMSCRIPTEN
            basePath = "/bin/";
        #else
            basePath = SDL_GetBasePath();
        #endif
    }

    /* Init TTF and any fonts */
    TTF_Init();
    dbgFont = TTF_OpenFont((basePath+"res/BackToEarth.ttf").c_str(), 100);
    if(dbgFont==NULL) {
        Log::errorv(__PRETTY_FUNCTION__, TTF_GetError(), "Could not open font.");
    }
    for(int i = 0; i<6; i++) {
        Text* t = new Text();
        t->init(rend, dbgFont, true);
        t->forcedNearestScaling(true);
        dbgScreen.pushBack(t);
    }

    MainLoopDriver mainLoop(&tick, 60, &draw, 300);
    Log::log("Quitting");

    GLSDL_GL_SaveState(rend);
    dbgScreen.clear();
    GLSDL_GL_RestoreState(rend);
    return 0;
}

#if ( defined(_WIN32) || defined(WIN32) )
int WINAPI WinMain()
{
    char** x = new char*[1];
    return main(0, x);
}
#endif
#include <iostream>
#include <nch/cpp-utils/arraylist.h>
#include <nch/cpp-utils/fs-utils.h>
#include <nch/cpp-utils/color.h>
#include <nch/cpp-utils/log.h>
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

bool doBackground = true;
bool firstDraw = true;
int64_t tickTimer = -10;
int64_t drawTimer = -10;
SDL_Texture* tex = nullptr;
SDL_Window* win = nullptr;
uint32_t winPixFormat = 0;
nch::ArrayList<nch::Text> dbgScreen;
TTF_Font* dbgFont = nullptr;
std::string basePath = "";

int getWidth() {
    int width = 0;
    SDL_GetWindowSize(win, &width, NULL);
    return width;
}
int getHeight() {
    int height = 0;
    SDL_GetWindowSize(win, NULL, &height);
    return height;
}

void drawInfo(SDL_Renderer* rend)
{
    int width = getWidth();
    int height = getHeight();

    std::stringstream s1; s1 << "CurrentTimeNS=" << nch::Timer::getCurrentTimeNS();
    dbgScreen[0].setText(s1.str());
    
    dbgScreen[1].setText(nch::MainLoopDriver::getPerformanceInfo());

    std::stringstream s2; s2 << "Window dimensions (W x H) = " << width << " x " << height << ".";
    dbgScreen[2].setText(s2.str());

    std::stringstream s3; s3 << "Last Known Input Event ID: " << nch::Input::getLastKnownSDLEventID();
    dbgScreen[3].setText(s3.str());
    
    dbgScreen[4].setText( nch::SDLEventDebugger::toString(nch::Input::getLastKnownSDLEvent()) );

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
    
    std::stringstream s4; s4 << "Battery: " << pct << "% (~" << secs << "s left). Power State: " << powerState;
    dbgScreen[4].setText(s4.str());

    double scale = 0.125*1;
    for(int i = 0; i<dbgScreen.size(); i++) {
        dbgScreen.at(i).setScale(scale);
    }
    

    for(int i = 0; i<dbgScreen.size(); i++) {
        dbgScreen.at(i).draw(width/2-dbgScreen.at(i).getWidth()/2, height/3+dbgScreen.at(0).getHeight()*i);
    }
}

void draw(SDL_Renderer* rend)
{
    //Only on first draw, create a clear texture that is 640x480.
    if(firstDraw) {
        if(doBackground) {
            tex = SDL_CreateTexture(rend, winPixFormat, SDL_TEXTUREACCESS_TARGET, 640, 480);
            nch::TexUtils::clearTexture(rend, tex);
        }
        firstDraw = false;
    }

    //Reset screen
    SDL_RenderClear(rend);
    SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
    SDL_RenderFillRect(rend, NULL);

    if(doBackground) {
        //Update parts of texture depending on timer
        SDL_SetRenderTarget(rend, tex);
        if(drawTimer>=0 && drawTimer<=480) {
            nch::Color c(255, 255, 255);
            int iy = drawTimer;
            for(int ix = 0; ix<640; ix++) {
                int i = iy*640+ix;
                c.setFromHSV(std::abs(ix+i/1000)%360, 100*iy/480, 100-(100*iy/480));
                SDL_SetRenderDrawColor(rend, c.r, c.g, c.b, 255);
                SDL_RenderDrawPoint(rend, ix, iy);
            }
        }
        SDL_SetRenderTarget(rend, NULL);

        //Draw texture and give it a color depending on the current tick timer
        nch::Color c2(255, 255, 255);
        c2.setFromHSV( std::abs(tickTimer%360), std::abs(tickTimer/3)%100, 100 );
        SDL_SetTextureColorMod(tex, c2.r, c2.g, c2.b);
        SDL_RenderCopy(rend, tex, NULL, NULL);
    }

    drawInfo(rend);

    //Render all present objects
    SDL_RenderPresent(rend);

    //Increment draw timer
    drawTimer++;
}

void tick()
{
    //Increment tick timer
    tickTimer++;
}

int main(int argc, char **argv)
{
    /* Say hello */
    nch::Log::log("Hello world");

    /* Init SDL, create window and renderer */
    SDL_Renderer* rend;
    {
        //SDL
        if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK)<0) {
            printf("SDL_Init Error: %s\n", SDL_GetError());
        }
        //Window
        win = SDL_CreateWindow("NCH-CPP-Utils Example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_RESIZABLE);
        if(win==NULL) {
            printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        }
        SDL_RaiseWindow(win);
        winPixFormat = SDL_GetWindowPixelFormat(win);
        //Renderer
        rend = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
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
        nch::Log::errorv(__PRETTY_FUNCTION__, TTF_GetError(), "Could not open font.");
    }
    for(int i = 0; i<6; i++) {
        nch::Text* t = new nch::Text();
        t->init(rend, dbgFont, true);
        t->forcedNearestScaling(true);
        dbgScreen.pushBack(t);
    }

    nch::MainLoopDriver mainLoop(rend, &tick, 60, &draw, 300);
    nch::Log::log("Quitting");
    return 0;
}

#if ( defined(_WIN32) || defined(WIN32) )
int WINAPI WinMain()
{
    char** x = new char*[1];
    return main(0, x);
}
#endif
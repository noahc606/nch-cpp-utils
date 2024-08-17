#include <iostream>
#include <nch/cpp-utils/gfx/Color.h>
#include <nch/sdl-utils/debug/SDLEventDebugger.h>
#include <nch/sdl-utils/gfx/Text.h>
#include <nch/sdl-utils/gfx/TexUtils.h>
#include <nch/sdl-utils/Input.h>
#include <nch/sdl-utils/MainLoopDriver.h>
#include <SDL2/SDL.h>
#include <sstream>

bool firstDraw = true;
int64_t tickTimer = -10;
int64_t drawTimer = -10;
SDL_Texture* tex = nullptr;
SDL_Window* win = nullptr;
uint32_t winPixFormat = 0;
Text dbgTxt0;
Text dbgTxt1;
TTF_Font* dbgFont = nullptr;

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
    double scale = getWidth()/640.;
    if(getHeight()/480.<scale) {
        scale = getHeight()/480.;
    }

    dbgTxt0.setText( SDLEventDebugger::toString(Input::getLastKnownSDLEvent()) );
    std::stringstream ss; ss << "Window dimensions (W x H) = " << width << " x " << " " << height << "."; dbgTxt1.setText(ss.str());
    dbgTxt0.setScale(scale/3);
    dbgTxt1.setScale(scale/3);

    dbgTxt0.draw(width/2-dbgTxt0.getWidth()/2, height/2-dbgTxt1.getHeight());
    dbgTxt1.draw(width/2-dbgTxt1.getWidth()/2, height/2-dbgTxt1.getHeight()*2);
}

void draw(SDL_Renderer* rend)
{
    //Only on first draw, create a clear texture that is 640x480.
    if(firstDraw) {
        tex = SDL_CreateTexture(rend, winPixFormat, SDL_TEXTUREACCESS_TARGET, 640, 480);
        TexUtils::clearTexture(rend, tex);
        firstDraw = false;
    }

    //Reset screen
    SDL_RenderClear(rend);
    SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
    SDL_RenderFillRect(rend, NULL);

    //Update parts of texture depending on timer
    SDL_SetRenderTarget(rend, tex);
    if(drawTimer>=0 && drawTimer<=480) {
        Color c(255, 255, 255);
        int iy = drawTimer;
        for(int ix = 0; ix<640; ix++) {
            int i = iy*640+ix;
            c.setFromHSV((ix+i/1000)%360, 100*iy/480, 100-(100*iy/480));
            SDL_SetRenderDrawColor(rend, c.r, c.g, c.b, 255);
            SDL_RenderDrawPoint(rend, ix, iy);
        }
    }
    SDL_SetRenderTarget(rend, NULL);

    //Draw texture and give it a color depending on the current tick timer
    Color c2(255, 255, 255);
    c2.setFromHSV( tickTimer%360, (tickTimer/3)%100, 100 );
    SDL_SetTextureColorMod(tex, c2.r, c2.g, c2.b);
    SDL_RenderCopy(rend, tex, NULL, NULL);

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

int main()
{
    /* Say hello */
    printf("Hello world\n");

    /* Init SDL, create window and renderer */
    //SDL
    if(SDL_Init(SDL_INIT_VIDEO)<0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
    }
    //Window
    win = SDL_CreateWindow("nchUtils Test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_RESIZABLE);
    if(win==NULL) {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
    }
    SDL_RaiseWindow(win);

    winPixFormat = SDL_GetWindowPixelFormat(win);
    //Renderer
    SDL_Renderer* rend = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    if(rend==NULL) {
        printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
    }

    /* Init TTF */
    TTF_Init();
    dbgFont = TTF_OpenFont("res/FreeMono.ttf", 72);
    dbgTxt0.init(rend, dbgFont, true);
    dbgTxt1.init(rend, dbgFont, true);

    /* Create main loop */
    MainLoopDriver mainLoop(rend, &tick, 60, &draw, 1000);
}
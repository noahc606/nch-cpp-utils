#include "NTexture.h"

using namespace nch;

int NTexture::pows[] = { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048 };

NTexture::NTexture(SDL_Renderer* rend, int w, int h)
{
    NTexture::rend = rend;
    pf = SDL_PIXELFORMAT_RGBA8888;

    resize(w, h);
    texmap = SDL_CreateTexture(rend, pf, SDL_TEXTUREACCESS_TARGET, mapW, mapH);
}
NTexture::~NTexture(){}

SDL_Texture* NTexture::getRawTex()
{
    return texmap;
}

void NTexture::destroy()
{
    if(texmap!=nullptr) {
        SDL_DestroyTexture(texmap);
        texmap = nullptr;
    }
}

void NTexture::resize(int w, int h)
{
    NTexture::texW = w;
    NTexture::texH = h;

    if(texW<0)    texW = 1;    if(texH<0)    texH = 1;
    if(texW>2048) texW = 2048; if(texH>1024) texH = 2048;

    mapW = w+(w/2);
    mapH = h;

    construct();
}

void NTexture::lock(SDL_Rect lr)
{
    //Get points making up the texture rectangle (NOT 'texmap' which is the entire mipmap)
    int x1 = lr.x;      int y1 = lr.y;
    int x2 = lr.x+lr.w; int y2 = lr.y+lr.h;

    //Cut these points off at the texture's edges (0,0) -> (texW,texH)
    if(x1<0) { x1 = 0; } else if(x1>texW) { x1 = texW; }
    if(x2<0) { x2 = 0; } else if(x2>texW) { x2 = texW; }
    if(y1<0) { y1 = 0; } else if(y1>texH) { y1 = texH; }
    if(y2<0) { y2 = 0; } else if(y2>texH) { y2 = texH; }

    //Set lockR
    if(x1<x2) { lockR.x = x1; } else { lockR.x = x2; }
    if(y1<y2) { lockR.y = y1; } else { lockR.y = y2; }
    lockR.w = std::abs(x1-x2);
    lockR.h = std::abs(y1-y2);
}

/*
    Each blit is actually a series of blits of different sizes
*/
void NTexture::blit(NTexture* srcNTex, const SDL_Rect& srcR)
{
    SDL_Texture* srcTex = srcNTex->getRawTex();

    

}


void NTexture::construct(){}
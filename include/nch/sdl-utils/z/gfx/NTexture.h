#pragma once
#include <SDL2/SDL.h>
#include <vector>
#include "Color.h"

/*
    An SDL_Texture* (TEXTURE_ACCESS_TARGET) implementation which has extra features:
    - Stores blit steps - able to rebuild texture from pre-loaded resource NTextures.
    - Mipmapping
*/

namespace nch { class NTexture {
public:
    struct BlitDef {
        SDL_Texture* srcTex = nullptr;
        SDL_Rect srcR;
        Color srcMod;

        SDL_Rect dstR;
    };


    NTexture(SDL_Renderer* rend, int w, int h);
    ~NTexture();

    SDL_Texture* getRawTex();

    void destroy();

    void resize(int w, int h);

    void lock(SDL_Rect lr);
    void blit(NTexture* ntex, const SDL_Rect& src);

private:
    void construct();
    static int pows[];

    SDL_Renderer* rend = nullptr;
    SDL_PixelFormatEnum pf;
    SDL_Texture* texmap = nullptr;
    std::vector<BlitDef> blits;
    int texW = 0, texH = 0, mapW = 0, mapH = 0;
    
    SDL_Rect lockR;
    SDL_Rect dst;
}; }
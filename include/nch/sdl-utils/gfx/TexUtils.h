#pragma once
#include <SDL2/SDL.h>

namespace nch { class TexUtils {
public:
    static void clearTexture(SDL_Renderer* rend, SDL_Texture*& tex);
    static void renderFillBorderedRect(SDL_Renderer* rend, SDL_Rect* r, float borderSize);
    static void renderFillTri(SDL_Renderer* rend, SDL_FPoint p0, SDL_FPoint p1, SDL_FPoint p2);
    static void renderFillTri(SDL_Renderer* rend, int x1, int y1, int x2, int y2, int x3, int y3);

private:
};}
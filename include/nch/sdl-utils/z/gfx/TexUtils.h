#pragma once
#include <SDL2/SDL.h>
#include "nch/cpp-utils/color.h"

namespace nch { class TexUtils {
public:
    static void clearTexture(SDL_Renderer* rend, SDL_Texture*& tex);
    static void renderFillBorderedRect(SDL_Renderer* rend, SDL_Rect* r, int borderSize, nch::Color borderColor);
    static void renderFillBorderedRect(SDL_Renderer* rend, SDL_Rect* r, int borderSize);
    static void renderFillTri(SDL_Renderer* rend, SDL_Point p0, SDL_Point p1, SDL_Point p2);
    static void renderFillTri(SDL_Renderer* rend, int x1, int y1, int x2, int y2, int x3, int y3);

private:
};}
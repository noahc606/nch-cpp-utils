#pragma once
#include <SDL2/SDL.h>

class TexUtils {
public:
    TexUtils();
    ~TexUtils();

    static void clearTexture(SDL_Renderer* rend, SDL_Texture*& tex);

private:

};
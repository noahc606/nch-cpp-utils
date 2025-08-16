#pragma once
#include <string>
#include <SDL2/SDL.h>

namespace nch { class SDLEventDebugger
{
public:
    static std::string toString(SDL_Event e);

private:

};
}
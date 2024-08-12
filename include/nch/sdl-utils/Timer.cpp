#include <nch/sdl-utils/Timer.h>
#include <SDL2/SDL.h>

/**
 * Get the time since startup in nanoseconds, according to SDL_GetPerformanceCounter() and SDL_GetPerformanceFrequency().
 */
uint64_t Timer::getCurrentTimeNS()
{
	//To avoid Uint64 overflow: multiply performance counter by 10k, divide by performance frequency, then multiply by 100k.
	//Effectively we have multiplied by 1B but some accuracy is lost.
	return SDL_GetPerformanceCounter()*(int64_t)10000/SDL_GetPerformanceFrequency()*100000;
}
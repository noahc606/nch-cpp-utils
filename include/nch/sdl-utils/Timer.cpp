#include <nch/cpp-utils/io/Log.h>
#include <nch/sdl-utils/Timer.h>
#include <SDL2/SDL.h>


NCH_Timer::NCH_Timer(std::string p_desc, bool p_logging)
{
    //Set description
    desc = p_desc;
    logging = p_logging;

    //Start timer
    t0 = getCurrentTime();
}
NCH_Timer::NCH_Timer(std::string p_desc): NCH_Timer::NCH_Timer(p_desc, false){}
NCH_Timer::NCH_Timer(): NCH_Timer::NCH_Timer("Generic timer", false){}

NCH_Timer::~NCH_Timer()
{
    //Update elapsed time
    updateElapsedTime();

    //Log timer message
    if( logging ) {
        debugElapsedTimeMS();
    }
}
/**/

/**
 * Get the time since startup in nanoseconds, according to SDL_GetPerformanceCounter() and SDL_GetPerformanceFrequency().
 */
uint64_t NCH_Timer::getCurrentTimeNS()
{
	//To avoid Uint64 overflow: multiply performance counter by 10k, divide by performance frequency, then multiply by 100k.
	//Effectively we have multiplied by 1B but some accuracy is lost.
	return SDL_GetPerformanceCounter()*(int64_t)10000/SDL_GetPerformanceFrequency()*100000;
}

uint64_t NCH_Timer::getTicks64()
{
    #if ( (SDL_MAJOR_VERSION>2) || (SDL_MAJOR_VERSION==2 && SDL_MINOR_VERSION>0) || (SDL_MAJOR_VERSION==2 && SDL_MINOR_VERSION==0 && SDL_PATCHLEVEL>=18))
        return SDL_GetTicks64();
    #endif
    return SDL_GetTicks();
}

double NCH_Timer::getElapsedTimeMS()
{
    //If timer hasn't finished, get elapsed time so far.
    if( dT==-1.0 ) {
        updateElapsedTime();
    }

    return dT;
}

void NCH_Timer::debugElapsedTimeMS()
{
    std::stringstream ss;
    ss << getElapsedTimeMS();
    Log::log("Finished "+desc+" in "+ss.str()+"ms.");
}

uint64_t NCH_Timer::getCurrentTime() { return SDL_GetPerformanceCounter(); }

void NCH_Timer::updateElapsedTime()
{
    //End timer
    t1 = getCurrentTime();
    //Calculate time elapsed (end time - initial time)
    dT = (t1-t0)*1000.0/(double)SDL_GetPerformanceFrequency();
}



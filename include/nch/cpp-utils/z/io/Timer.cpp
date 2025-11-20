#include "nch/cpp-utils/log.h"
#include "nch/cpp-utils/timer.h"
#include <chrono>
#include <thread>

using namespace nch;

TimerInit Timer::ti;

Timer::Timer(std::string desc, bool logging)
{
    //Set description
    Timer::desc = desc;
    Timer::logging = logging;

    //Start timer
    t0 = getCurrentTimeNS();
}
Timer::Timer(std::string p_desc): Timer::Timer(p_desc, false){}
Timer::Timer(): Timer::Timer("Generic timer", false){}

Timer::~Timer()
{
    //Log timer message
    if(logging) {
        debugElapsedTimeMS();
    }
}
/**/

/**
 * Get the time since startup in nanoseconds, according to TimerInit.
 */
uint64_t Timer::getCurrentTimeNS() { return ti.nsSinceStartup(); }
/**
 * Get the time since startup in milliseconds.
 */
uint64_t Timer::getTicks() { return getCurrentTimeNS()/(uint64_t)1000000; }

double Timer::getElapsedTimeMS()
{
    //End timer
    t1 = getCurrentTimeNS();
    //Calculate time elapsed and convert to milliseconds.
    dT = (double)((t1-t0)/(double)1000000);
    return dT;
}

void Timer::debugElapsedTimeMS()
{
    std::stringstream ss;
    ss << getElapsedTimeMS();
    Log::log("Finished "+desc+" in "+ss.str()+"ms.");
}

/// @brief Sleep for the specified number of milliseconds.
/// @param ms Number of milliseconds to sleep for.
void Timer::sleep(int ms)
{
    if(ms<0) ms = 0;
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
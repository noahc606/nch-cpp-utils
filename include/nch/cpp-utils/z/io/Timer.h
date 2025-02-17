#pragma once
#include <cstdint>
#include <string>
#include "TimerInit.h"

namespace nch { class Timer {
public:
    
    Timer(std::string desc, bool logging);
    Timer(std::string desc);
    Timer();
    virtual ~Timer();
    
    static uint64_t getCurrentTimeNS();
    static uint64_t getTicks();
    double getElapsedTimeMS();

    void debugElapsedTimeMS();
    static void sleep(int ms);
private:
    uint64_t getCurrentTime();
    
    void updateElapsedTime();
    
    /* Times (in nanoseconds) */
    uint64_t t0 = 0;    //Initial time (time constructor was called)
    uint64_t t1 = 0;    //End time (time destructor activates)
    /* Time Elapsed (in milliseconds) */
    double dT = -1.0;   //Time elapsed between t0 and t1

    /* Peripherals */
    bool logging = true;
    std::string desc = "Description unuset";
    static TimerInit ti;
    /**/
};}
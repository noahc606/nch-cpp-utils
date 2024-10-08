#pragma once
#include <cstdint>
#include <string>

namespace nch { class Timer {
public:
    
    Timer(std::string desc, bool logging);
    Timer(std::string desc);
    Timer();
    virtual ~Timer();
    
    static uint64_t getCurrentTimeNS();
    static uint64_t getTicks64();
    double getElapsedTimeMS();
    
    void debugElapsedTimeMS();

private:
    uint64_t getCurrentTime();
    
    void updateElapsedTime();
    
    /* Times (in milliseconds) */
    uint64_t t0 = 0.0;    //Initial time
    uint64_t t1 = 0.0;    //End time
    double dT = -1.0;   //Time elapsed

    /* Peripherals */
    bool logging = true;
    std::string desc = "Description unuset";
    /**/
};}
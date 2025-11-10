#include "TimerInit.h"

TimerInit::TimerInit()
{
    clock_gettime(CLOCK_MONOTONIC, &ts);
    startupNS = (int64_t)ts.tv_sec * (int64_t)1000000000 + (int64_t)ts.tv_nsec;
}
TimerInit::~TimerInit(){}

uint64_t TimerInit::nsSinceStartup()
{
    clock_gettime(CLOCK_MONOTONIC, &ts);
    int64_t nowNS = (int64_t)ts.tv_sec * (int64_t)1000000000 + (int64_t)ts.tv_nsec;
    return (nowNS-startupNS);
}
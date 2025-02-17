#pragma once
#include <cstdint>
#include <time.h>

class TimerInit {
public:
    TimerInit();
    ~TimerInit();

    uint64_t nsSinceStartup();
private:
    struct timespec ts;
    int64_t startupNS;
};
#include "Shell.h"
#include <array>
#include <chrono>
#include <future>
#include <memory>
#include <stdexcept>
#include <stdio.h>
#include <thread>
#include <unistd.h>
#ifdef _UNISTD_H
    #include <unistd.h>
#endif
#include "nch/cpp-utils/log.h"
#include "nch/cpp-utils/timer.h"

using namespace nch;

std::string Shell::exec(const char* cmd) {    
    std::array<char, 128> buffer;
    std::stringstream result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if(!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    
    int numIterations = 0;
    auto bufData = buffer.data();
    int bufSize = static_cast<int>(buffer.size());
    while(fgets(bufData, bufSize, pipe.get()) != nullptr) {
        result << buffer.data();
        numIterations++;
    }
    return result.str();
}

std::string Shell::execWithTimeout(std::string cmd, int timeoutMS) {
    //Promise and future
    std::promise<std::string> resultPromise;
    std::future<std::string> resultFuture = resultPromise.get_future();
    //Thread function using promise
    auto thdFunc = [](std::string p_cmd, std::promise<std::string> p_result) {
        std::string tempRes = Shell::exec(p_cmd);
        p_result.set_value(tempRes);
    };
    
    //Create thread, and wait for 'timeoutMS' milliseconds
    std::thread cmdThd(thdFunc, cmd, std::move(resultPromise));
    Timer::sleep(timeoutMS);

    //Get result
    if(resultFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
        cmdThd.join();
        return resultFuture.get();
    } else {
        cmdThd.detach();
        return "???null???";
    }
}

std::string Shell::exec(std::string cmd) { return exec(cmd.c_str()); }

#ifdef _UNISTD_H
int Shell::cd(const char* path) { return chdir(path); }
int Shell::cd(std::string path) { return cd(path.c_str()); }
#else
int Shell::cd(const char* path) { nch::Log::error(__PRETTY_FUNCTION__, "unistd.h does not exist on this system, doing nothing"); return -1; }
int Shell::cd(std::string path) { return cd(path.c_str()); }
#endif

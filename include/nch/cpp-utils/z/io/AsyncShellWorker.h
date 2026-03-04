#pragma once
#include <mutex>
#include <string>
#include <thread>

namespace nch { class AsyncShellWorker {
public:
    struct ExecJob {
        std::string signal = "";
        std::string cmd = "";
    };
    struct ExecResult {
        bool exists = false;
        std::string data = "";
        std::string signal = "";
    };

    AsyncShellWorker(); ~AsyncShellWorker();

    bool run(ExecJob& ej);
    bool isRunning();
    bool isFetchValid();
    ExecResult pop();
private:
    ExecResult execResult = ExecResult();
    bool execValid = false;
    bool threadRunning = false;
    std::mutex mtx;
    std::thread execThread;
    ExecJob execJob;
}; }

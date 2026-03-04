#include "AsyncShellWorker.h"
#include "nch/cpp-utils/fs-utils.h"
#include "nch/cpp-utils/log.h"
#include "nch/cpp-utils/shell.h"
#include "nch/cpp-utils/string-utils.h"
#include "nch/cpp-utils/timer.h"
using namespace nch;

AsyncShellWorker::AsyncShellWorker(){}
AsyncShellWorker::~AsyncShellWorker()
{
    try { execThread.detach(); } catch(...) {}
}

bool AsyncShellWorker::run(ExecJob& ej)
{
    //If a previous thread is still running OR a previous worker has not been popped, do nothing.
    if(isRunning() || isFetchValid()) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(mtx);
        AsyncShellWorker::execJob = ej;
    }

    if(execThread.joinable()) execThread.join();
    try {
        execThread = std::thread([this]() {
            {
                std::lock_guard<std::mutex> lock(mtx);
                threadRunning = true;
            }
            ExecResult ret; {
                ret.data = Shell::exec(execJob.cmd);
                ret.signal = execJob.signal;
                ret.exists = true;
            }
            {
                std::lock_guard<std::mutex> lock(mtx);
                execValid = true;  //Mark that 'execResult' is ready
                threadRunning = false;
                execResult = ret;
                execThread.detach();
            }
        });

        return true;
    } catch(...) {}
    
    return false;
}

bool AsyncShellWorker::isRunning() {
    std::lock_guard<std::mutex> lock(mtx);
    return threadRunning;
}
bool AsyncShellWorker::isFetchValid() {
    std::lock_guard<std::mutex> lock(mtx);
    return execValid;
}
AsyncShellWorker::ExecResult AsyncShellWorker::pop() {
    std::lock_guard<std::mutex> lock(mtx);
    if(execValid) {
        ExecResult ret = execResult;
        execResult = ExecResult();  //Reset the 'execResult' value after retrieving it
        execValid = false;          //Mark that 'execResult' is no longer ready
        return ret;
    }
    return ExecResult(); //Return an empty WFR if fetch is not ready yet
}

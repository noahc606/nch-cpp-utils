#pragma once
#include <queue>
#include <stack>
#include <string>
#include "AsyncShellWorker.h"

namespace nch { class AsyncShell {
public:
    AsyncShell(bool cmdExecMessages = false);
    ~AsyncShell();

    void tickExec();
    void pushExec(const std::string& cmd, std::string signal = "");
    nch::AsyncShellWorker::ExecResult popResult();

private:
    void tickAssignWorkFromStacks();
    void msgStartedJob(const nch::AsyncShellWorker::ExecJob& ej);
    void msgFinishedJob(const nch::AsyncShellWorker::ExecJob& ej, const nch::AsyncShellWorker::ExecResult& er);

    bool cmdExecMessages = false;
    nch::AsyncShellWorker execWkr;
    std::queue<nch::AsyncShellWorker::ExecResult> execResults;
    std::queue<nch::AsyncShellWorker::ExecJob> cmdExecJobsLeft;
    int64_t workAssignWaitTicksLeft = 0;
}; }
#include "AsyncShell.h"
#include <cinttypes>
#include "nch/cpp-utils/log.h"
#include "nch/cpp-utils/string-utils.h"
#include "nch/cpp-utils/timer.h"
using namespace nch;

AsyncShell::AsyncShell(bool cmdExecMessages) {
    AsyncShell::cmdExecMessages = cmdExecMessages;
}
AsyncShell::~AsyncShell() {}

void AsyncShell::tickExec() {
    if(workAssignWaitTicksLeft==0) {
        tickAssignWorkFromStacks();
    }
    if(workAssignWaitTicksLeft>0) {
        workAssignWaitTicksLeft--;
    }
}

void AsyncShell::pushExec(const std::string& cmd, std::string signal) {
    AsyncShellWorker::ExecJob ej; {
        ej.cmd = cmd;
        ej.signal = signal;
    }
    cmdExecJobsLeft.push(ej);
}

AsyncShellWorker::ExecResult AsyncShell::popResult() {
    AsyncShellWorker::ExecResult ret;
    if(execResults.size()>0) {
        ret = execResults.front();
        execResults.pop();
    }
    return ret;
}

void AsyncShell::tickAssignWorkFromStacks()
{
    /* Handle 'execWkr's */
    {
        AsyncShellWorker::ExecResult pop = execWkr.pop();
        if(pop.exists)
        {   //Retrieve any existing finished job
            AsyncShellWorker::ExecJob topJob = cmdExecJobsLeft.front();
            cmdExecJobsLeft.pop();
            execResults.push(pop);
                msgFinishedJob(topJob, pop);
            workAssignWaitTicksLeft = 2;
        } else if(cmdExecJobsLeft.size()>0)
        {   //Try to start a new job if it is available
            AsyncShellWorker::ExecJob topJob = cmdExecJobsLeft.front();
            if(execWkr.run(topJob)) {
                msgStartedJob(topJob);
            }
            workAssignWaitTicksLeft = 2;
        }
    }
}

void AsyncShell::msgStartedJob(const AsyncShellWorker::ExecJob& ej)
{
    if(!cmdExecMessages) return;
    int left = cmdExecJobsLeft.size()-1;

    Log::log("[time=%" PRIu64 "ms] Executed job (%d left after this one): \"%s\".",
        Timer::getTicks(),
        left,
        StringUtils::shortened(ej.cmd, 24).c_str()
    );
}
void AsyncShell::msgFinishedJob(const AsyncShellWorker::ExecJob& ej, const AsyncShellWorker::ExecResult& er)
{
    if(!cmdExecMessages) return;
    if(er.signal=="") {
        Log::log("[time=%" PRIu64 "ms] Shell::exec(\"%s\") returned \"%s\".",
            Timer::getTicks(),
            StringUtils::shortened(ej.cmd, 24).c_str(),
            StringUtils::shortened(er.data, 32).c_str()
        );
        return;
    }

    Log::log("[time=%" PRIu64 "ms] Shell::exec(\"%s\") returned \"%s\" w/ signal %s.",
        Timer::getTicks(),
        StringUtils::shortened(ej.cmd, 24).c_str(),
        StringUtils::shortened(er.data, 32).c_str(),
        er.signal.c_str()
    );
}

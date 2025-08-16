#pragma once
#include <string>

namespace nch { class Shell {
public:
    /// @brief Execute a shell command synchronously.
    /// @brief This will hang (wait until execution finishes) the entire thread from which this was called.
    /// @param cmd The shell command to execute.
    /// @return The resulting standard output after running the command.
    static std::string exec(const char* cmd);
    static std::string exec(std::string cmd);

    /// @brief Execute a shell command asynchronously and wait a specified amount of time to get the results.
    /// @brief This will always hang the thread from which this was called for 'timeoutMS' milliseconds.
    /// @brief Note that this may leak resources if commands are run that never exit properly. An 'unsafe' way to get around this is by using Shell::exec("kill <pname>") after using this function.
    /// @param cmd The shell command to execute.
    /// @param timeoutMS The number of milliseconds to wait to get the result of the shell command.
    /// @return The resulting standard output after running the command, or "???null???" if the shell command took too long ("timed out").
    static std::string execWithTimeout(std::string cmd, int timeoutMS);

    static int cd(const char* path);
    static int cd(std::string path);
private:
    
}; }

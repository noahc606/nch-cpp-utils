#include "Shell.h"
#include <array>
#include <memory>
#include <stdexcept>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
    #include <unistd.h>
#endif
#include "nch/cpp-utils/log.h"

using namespace nch;

std::string Shell::exec(const char* cmd) {    
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

std::string Shell::exec(std::string cmd) { return exec(cmd.c_str()); }

#ifdef HAVE_UNISTD_H
int Shell::cd(const char* path) { return chdir(path); }
int Shell::cd(std::string path) { return cd(path.c_str()); }
#else
int Shell::cd(const char* path) { nch::Log::error(__PRETTY_FUNCTION__, "unistd.h does not exist on this system, doing nothing"); return -1; }
int Shell::cd(std::string path) { return cd(path.c_str()); }
#endif

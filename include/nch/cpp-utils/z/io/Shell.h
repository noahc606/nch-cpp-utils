#pragma once
#include <string>

class Shell {
public:
    static std::string exec(const char* cmd);
    static std::string exec(std::string cmd);
    static int cd(const char* cmd);
    static int cd(std::string path);
private:
    
};

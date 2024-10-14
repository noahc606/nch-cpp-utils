#pragma once
#include <string>

namespace nch { class FilePath {
public:
    FilePath(std::string path);

    std::string get();
    std::string getObjectName(bool includeExtension);
    std::string getObjectName();
    std::string getGrandparentDir(int numUpDirs);
    std::string getParentDir();
    int getNumDirsDown();
    std::string getExtension();
    std::string getWithoutExtension();

    static const std::string validChars;
private:
    std::string cleanpath = "";
    /* data */
};}
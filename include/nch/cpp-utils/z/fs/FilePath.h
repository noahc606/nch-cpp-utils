#pragma once
#include <string>

namespace nch { class FilePath {
public:
    FilePath(std::string path);

    std::string get();
    std::string getFilename(bool includeExtension);
    std::string getFilename();
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
#pragma once
#include <string>

namespace nch { class FilePath {
public:
    FilePath(std::string path);

    std::string get();
    std::string getObjectName(bool includeExtension);
    std::string getObjectName();
    std::string getGrandparentDirName(int numUpDirs);
    std::string getParentDirName(); std::string getParentDirPath();
    int getNumDirsDown();
    std::string getExtension();
    std::string getWithoutExtension();
    bool isHidden();

    static const std::string validChars;
private:
    std::string cleanpath = "";
    /* data */
};}
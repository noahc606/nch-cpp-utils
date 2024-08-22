#pragma once
#include <string>

class NCH_FilePath
{
public:
    NCH_FilePath(std::string path);

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
};

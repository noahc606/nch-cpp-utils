#pragma once
#include <string>
#include <vector>

class NCH_FsUtils {
public:
    static bool dirExists(std::string dirPath);
    static bool regularFileExists(std::string rfPath);
    static std::vector<std::string> listDirContents(std::string dirPath, int listType, bool recursive);
    static std::vector<std::string> listDirContents(std::vector<std::string> dirPaths, int listType, bool recursive);
    static std::string getPathWithInferredExtension(std::string path);
    enum ListTypes {
        ONLY_DIRS,
        ONLY_FILES,
        ALL
    };
private:

};
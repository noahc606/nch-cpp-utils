#pragma once
#include <string>
#include <vector>

namespace nch { class FsUtils {
public:
    static int createDir(std::string path);
    static bool pathExists(std::string path);
    static bool dirExists(std::string dirPath);
    static bool fileExists(std::string rfPath);
    static std::vector<std::string> ls(std::string dirPath, int maxItemsToList);
    static std::vector<std::string> ls(std::string dirPath);
    static std::vector<std::string> getDirContents(std::string dirPath, int listType, bool recursive, int maxItemsToList);
    static std::vector<std::string> getDirContents(std::string dirPath, int listType, bool recursive);
    static std::vector<std::string> getDirContents(std::string dirPath, int listType);
    static std::vector<std::string> getDirContents(std::string dirPath);
    static std::vector<std::string> getDirContents(std::vector<std::string> dirPaths, int listType, bool recursive);
    static std::string getPathWithInferredExtension(std::string path);
    enum ListTypes {
        ONLY_DIRS,
        ONLY_FILES,
        ALL
    };
private:
};}
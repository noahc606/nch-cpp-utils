#pragma once
#include <string>
#include <vector>

namespace nch { class FsUtils {
public:

    typedef struct {
        bool includeDirs = true;            //Normal dirs
        bool includeFiles = true;           //Normal files
        bool includeHiddenEntries = true;   //Hidden dirs and files (filename starts with a ".");

        bool excludeSymlinkDirs = false;    //Directories symlinked to another location
        int maxItemsToList = 1024;
    } ListSettings;

    typedef struct {
        bool recursiveSearch = false;
        std::vector<std::string> skippedDirPaths;
        int numLayersDown = 0;
        int maxLayersDown = 999;
    } RecursionSettings;

    static int createDir(std::string path);
    static bool pathExists(std::string path);
    static bool dirExists(std::string dirPath);
    static bool fileExists(std::string rfPath);
    static bool symlinkExists(std::string lnkPath);
    static std::vector<std::string> lsx(std::string dirPath, ListSettings& lise);
    static std::vector<std::string> ls(std::string dirPath, int maxItemsToList);
    static std::vector<std::string> ls(std::string dirPath);
    static std::vector<std::string> getDirContents(std::string dirPath, ListSettings& lise, RecursionSettings& rese);
    static std::vector<std::string> getDirContents(std::string dirPath, ListSettings& lise);
    static std::vector<std::string> getDirContents(std::string dirPath);
    static std::vector<std::string> getManyDirContents(std::vector<std::string> dirPaths, ListSettings& lise, RecursionSettings& rese);
    static std::string getPathWithInferredExtension(std::string path);

    static void setLogWarnings(bool lw);
private:
    static bool tryAddToDirentList(std::string dirPath, std::vector<std::string>& vec, std::string ent, ListSettings& lise);

    static bool logWarnings;
};}
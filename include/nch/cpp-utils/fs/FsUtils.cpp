#include "FsUtils.h"
#include "FilePath.h"
#include <filesystem>
#include <set>
#include <sstream>

FsUtils::FsUtils(){}
FsUtils::~FsUtils(){}

bool FsUtils::dirExists(std::string dirPath)
{
    if(std::filesystem::is_directory(dirPath)) return true;
    return false;
}

bool FsUtils::regularFileExists(std::string rfPath)
{
    if(std::filesystem::is_regular_file(rfPath)) return true;
    return false;
}

std::vector<std::string> FsUtils::listDirContents(std::string dirPath, int listType, bool recursive)
{
    std::string mfp = dirPath;

    std::vector<std::string> res;
    if(dirExists(dirPath)) {

        std::vector<std::filesystem::directory_entry> dev;  //Directory entry vector
        if(recursive) {
            using dritr = std::filesystem::recursive_directory_iterator;
            dritr dirListRec = dritr(mfp);
            for (const auto& dir : dirListRec) { dev.push_back(dir); }
        } else {
            using ditr = std::filesystem::directory_iterator;
            ditr dirList = ditr(mfp);
            for (const auto& dir : dirList) { dev.push_back(dir); }
        }

        for (const auto& de : dev) {
            //Get 'de' (directory entry) and remove all quotations (weird issue)
            std::stringstream ss0; ss0 << de;
            std::string s0 = ss0.str();
            std::stringstream ss;
            for(int i = 0; i<s0.size(); i++) {
                if(s0[i]!='\"') {
                    ss << s0[i];
                }
            }

            //Build filepath
            FilePath fp(ss.str());

            //Depending on listType, add files/directories/both to the list
            if(listType==ONLY_DIRS) {
                if(dirExists(fp.get())) {
                    res.push_back(fp.get());
                }
            } else if(listType==ONLY_FILES) {
                if(!dirExists(fp.get())) {
                    res.push_back(fp.get());
                }
            } else if(listType==ALL) {
                res.push_back(fp.get());
            } else {
                printf("Unknown listType, returning empty vector...");
                return res;
            }
        }

    } else {
        printf("Warning: \"%s\" is not a directory - returning empty vector.\n", dirPath.c_str());
    }


    return res;
}

std::vector<std::string> FsUtils::listDirContents(std::vector<std::string> dirPaths, int listType, bool recursive)
{
    std::vector<std::string> res;
    
    //Loop through all dirPaths
    for(int i = 0; i<dirPaths.size(); i++) {
        //Loop through this dirPath's list of strings.
        auto ldc = listDirContents(dirPaths[i], listType, recursive);
        for(std::string s : ldc) {
            //Make sure each of strings does not exist within res before adding it
            bool contained = false;
            for(std::string r : res) {
                if(s==r) {
                    contained = true;
                    break;
                }
            }
            //If the string is new, add it.
            if(contained==false) {
                res.push_back(s);
            }
        }
    }

    //Return the final list
    return res;
}

std::string FsUtils::getPathWithInferredExtension(std::string path) {
    int i = -1;
    for(i = path.size()-1; i>=0; i--) {
        if(path[i]=='/' || path[i]=='\\') {
            break;
        }
    }

    std::string res = "?null?";
    int count = 0;
    std::vector<std::string> dirFileList = listDirContents(path.substr(0, i), ListTypes::ONLY_FILES, false);
    for(std::string f : dirFileList) {
        FilePath fp(f);
        std::string ext = fp.getExtension();

        std::string potentialFile = path+"."+ext;
        if(path==fp.getWithoutExtension() && regularFileExists(potentialFile)) {
            res = potentialFile;
            count++;
        }
    }

    if(count==0) {
        return "?null?";
    } else if(count==1) {
        return res;
    }

    printf("Warning: found %d possible matches for \"%s\", returning \"%s\"...\n", count, path.c_str(), res.c_str());
    return res;
}
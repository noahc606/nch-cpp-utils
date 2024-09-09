#include "FsUtils.h"
#include <functional>
#include <set>
#include <sstream>
#include <unistd.h>
#include "FilePath.h"
#include "Log.h"

#if ( defined(_WIN32) || defined(WIN32) )
    #include "direct.h"
    #include "fileapi.h"
    #include <windows.h>
#elif ( defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)) )
    #include <dirent.h>
    #include <sys/stat.h>
#endif

using namespace nch;

int FsUtils::createDir(std::string path)
{
    return -100;
}

bool FsUtils::pathExists(std::string path)
{
    if(path.size()==0) return true;

    //Check if the path at 'path' exists...
    #if ( defined(_WIN32) || defined(WIN32) )
        // ...for Windows
        DWORD attrs = GetFileAttributesA(path.c_str());
        if(attrs!=INVALID_FILE_ATTRIBUTES) {
            return true;
        } else {
            return false;
        }
    #elif ( defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)) )
        // ...for Unix
		#if ( defined(_POSIX_VERSION) )
            // ...If POSIX supported (unistd.h)
            struct stat sb;
            if(stat(path.c_str(), &sb)==0) {
                if( !(sb.st_mode&S_IFDIR) && !(sb.st_mode&S_IFREG) ) {
	                Log::error(__PRETTY_FUNCTION__, "Object @ \"%s\" doesn't seem to be a dir or a normal file, returning false", path.c_str());
                    return false;
                }
                return true;
            }
		#else
            // ...Unix but POSIX unsupported
		    nch::Log::error(__PRETTY_FUNCTION__, "POSIX seemingly unsupported for this Unix OS, returning false");
		#endif
    #else
        // ...Not on Windows/Unix -> Error
		nch::Log::error(__PRETTY_FUNCTION__, "Unknown operating system, returning false. Are you on Windows/Unix?");
    #endif
        
    return false;
}

bool FsUtils::fileExists(std::string path)
{
    if(!pathExists(path)) return false;

    #if ( defined(_WIN32) || defined(WIN32) )
        DWORD attrs = GetFileAttributesA(path.c_str());
	    bool isDir = attrs&FILE_ATTRIBUTE_DIRECTORY;
	    bool isArchive = attrs&FILE_ATTRIBUTE_ARCHIVE;
        return (!isDir && !isArchive);
    #elif ( defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)) )
        struct stat sb;
        if((stat(path.c_str(), &sb)==0)) {
            return (sb.st_mode & S_IFREG);
        }
    #endif

    return false;
}

bool FsUtils::dirExists(std::string path)
{
    if(!pathExists(path)) return false;

    #if ( defined(_WIN32) || defined(WIN32) )
        DWORD attrs = GetFileAttributesA(path.c_str());
        return attrs&FILE_ATTRIBUTE_DIRECTORY;
    #elif ( defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)) )
        struct stat sb;
        if((stat(path.c_str(), &sb)==0)) {
            return (sb.st_mode & S_IFDIR);
        }
    #endif

    return false;
}

std::vector<std::string> FsUtils::ls(std::string dirPath, int maxItemsToList)
{
    if(dirPath=="") dirPath = ".";

    //Result to be returned
    std::vector<std::string> res;
    if(!dirExists(dirPath)) return res;

    #if ( defined(_WIN32) || defined(WIN32) )
        HANDLE hFind;
        WIN32_FIND_DATAA winFindData;
	    std::string wildcardPath = dirPath+"/*";
        if( (hFind = FindFirstFileA(wildcardPath.c_str(), &winFindData))!=INVALID_HANDLE_VALUE ) {
            do {
		        std::string entry = winFindData.cFileName;
                if(res.size()<maxItemsToList) {
     		        //If this is NOT the "." or ".." directory, add it.
               	    if(entry!="." && entry!="..") res.push_back(entry);
                }
            } while(FindNextFileA(hFind, &winFindData)!=0);
            FindClose(hFind);
        }
    #elif ( defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)) )
        //Open this directory...
        DIR* dir = opendir(dirPath.c_str());
        if(dir!=NULL) {
            //Go through all the entries of this directory...
            struct dirent* ent;
            while((ent = readdir(dir))!=NULL) {
		        std::string entry = ent->d_name;
                if(res.size()<maxItemsToList) {
     		        //If this is NOT the "." or ".." directory, we consider it.
               	    if(entry!="." && entry!="..") res.push_back(entry);
                }
            }
            
            if(closedir(dir)==-1) {
                Log::warn(__PRETTY_FUNCTION__, "Failed to close directory \"%s\" after operation", dirPath.c_str());
            }
        } else {
            Log::error(__PRETTY_FUNCTION__, "Could not open directory \"%s\", returning empty vector.", dirPath.c_str());
        }
    #endif

    return res;
}

std::vector<std::string> FsUtils::ls(std::string dirPath) { return ls(dirPath, 1024); }

std::vector<std::string> FsUtils::getDirContents(std::string dirPath, int listType, bool recursive, int maxItemsToList)
{
    //Result to be returned
    std::vector<std::string> res;

    //Go through all the entries of this directory...
    std::vector<std::string> lsRes = ls(dirPath, maxItemsToList);
    for(int i = 0; i<lsRes.size(); i++) {
        std::string entpath;
        if(dirPath.size()>0) {
            entpath = dirPath+"/"+lsRes[i];
        } else {
            entpath = lsRes[i];
        }

        //If doing recursive listing, add everything within any dir we come across.
        std::vector<std::string> subDirContents;
        if(recursive && dirExists(entpath)) {
            subDirContents = getDirContents(entpath, listType, true, maxItemsToList-res.size());
        }

        //Add this entry as well as everything in subDirContents.
        if(res.size()<maxItemsToList) {
            switch(listType) {
                case FsUtils::ONLY_DIRS: {
                    if(dirExists(entpath)) res.push_back(entpath);
                } break;
                case FsUtils::ONLY_FILES: {
                    if(fileExists(entpath)) res.push_back(entpath);
                } break;
                default: {
                    res.push_back(entpath);
                } break;
            }
	    }
        for(int i = 0; i<subDirContents.size(); i++) {
            if(res.size()<maxItemsToList) res.push_back(subDirContents[i]);
        }
    }

    return res;
}

std::vector<std::string> FsUtils::getDirContents(std::string dirPath, int listType, bool recursive) { return getDirContents(dirPath, listType, recursive, 1024); }
std::vector<std::string> FsUtils::getDirContents(std::string dirPath, int listType) { return getDirContents(dirPath, listType, false); }
std::vector<std::string> FsUtils::getDirContents(std::string dirPath) { return getDirContents(dirPath, FsUtils::ALL); }

std::vector<std::string> FsUtils::getDirContents(std::vector<std::string> dirPaths, int listType, bool recursive)
{
    std::vector<std::string> res;
    
    //Loop through all dirPaths
    for(int i = 0; i<dirPaths.size(); i++) {
        //Loop through this dirPath's list of strings.
        auto gdc = getDirContents(dirPaths[i], listType, recursive);
        for(std::string s : gdc) {
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
    std::vector<std::string> dirFileList;
    if(i==-1) {
        dirFileList = getDirContents("", ListTypes::ONLY_FILES, false);
    } else {
        dirFileList = getDirContents(path.substr(0, i), ListTypes::ONLY_FILES, false);
    }
    for(std::string f : dirFileList) {
        FilePath fp(f);
        std::string ext = fp.getExtension();

        std::string potentialFile = path+"."+ext;
        if(path==fp.getWithoutExtension() && fileExists(potentialFile)) {
            res = potentialFile;
            count++;
        }
    }

    if(count==0) {
        return "?null?";
    } else if(count==1) {
        return res;
    }

    Log::warnv(__PRETTY_FUNCTION__, "returning "+res, "Found %d possible matches for \"%s\"", count, path.c_str());
    return res;
}
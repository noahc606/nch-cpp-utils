#include "FsUtils.h"
#include <functional>
#include <set>
#include <sstream>
#include <unistd.h>
#include "FilePath.h"
#include "nch/cpp-utils/log.h"

#if ( defined(_WIN32) || defined(WIN32) )
    #include "direct.h"
    #include "fileapi.h"
    #include <windows.h>
#elif ( defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)) )
    #include <dirent.h>
    #include <sys/stat.h>
#endif

using namespace nch;

bool FsUtils::logWarnings = false;

int FsUtils::createDir(std::string path) {
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
                if( !((sb.st_mode&S_IFMT)==S_IFDIR) && !(sb.st_mode&S_IFREG) ) {
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

/// @brief Searches for a file at the given 'path' and returns true if it was found, false if not.
/// @param path The path of the potential file to look for.
/// @return Whether or not a file at 'path' exists.
bool FsUtils::fileExists(std::string path)
{
    if(!pathExists(path)) return false;

    #if ( defined(_WIN32) || defined(WIN32) )
        DWORD attrs = GetFileAttributesA(path.c_str());
	    bool isDir = attrs&FILE_ATTRIBUTE_DIRECTORY;
        return (!isDir);
    #elif ( defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)) )
        struct stat sb;
        if((stat(path.c_str(), &sb)==0)) {
            return (sb.st_mode&S_IFREG);
        }
    #endif

    return false;
}

/// @brief Searches for a directory at the given 'path' and returns true if it was found, false if not.
/// @param path The path of the potential directory to look for.
/// @return Whether or not a directory at 'path' exists.
bool FsUtils::dirExists(std::string path)
{
    if(!pathExists(path)) return false;

    #if ( defined(_WIN32) || defined(WIN32) )
        DWORD attrs = GetFileAttributesA(path.c_str());
        return attrs&FILE_ATTRIBUTE_DIRECTORY;
    #elif ( defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)) )
        struct stat sb;
        if(stat(path.c_str(), &sb)==0) {
            if((sb.st_mode&S_IFMT)==S_IFDIR)
                return true;
        }
    #endif

    return false;
}

/// @brief Searches for a symlink at the given 'path' and returns true if it was found, false if not.
/// @param path The path of the potential symlink to look for.
/// @return Whether or not a symlink at 'path' exists.
bool FsUtils::symlinkExists(std::string path)
{
    if(!pathExists(path)) return false;

    //Remove all trailing slashes at the end of 'path'
    int count = 0;
    for(int i = path.size()-1; i>=0; i--) {
        if(path[i]=='/' || path[i]=='\\') {
            count++;
        } else {
            break;
        }
    }
    path = path.substr(0, path.size()-count);    

    #if ( defined(_WIN32) || defined(WIN32) )
        DWORD attrs = GetFileAttributesA(path.c_str());
        return attrs&FILE_ATTRIBUTE_REPARSE_POINT;
    #elif ( defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)) )
        struct stat sb;
        if((lstat(path.c_str(), &sb)==0)) {
            return S_ISLNK(sb.st_mode);
        }
    #endif

    return false;
}

std::vector<std::string> FsUtils::lsx(std::string dirPath, ListSettings& lise)
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
                tryAddToDirentList(dirPath, res, entry, lise);
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
                tryAddToDirentList(dirPath, res, entry, lise);
            }
            
            if(closedir(dir)==-1) {
                if(logWarnings) Log::warn(__PRETTY_FUNCTION__, "Failed to close directory \"%s\" after operation", dirPath.c_str());
            }
        } else {
            Log::error(__PRETTY_FUNCTION__, "Could not open directory \"%s\", returning empty vector", dirPath.c_str());
        }
    #endif

    return res;
}

std::vector<std::string> FsUtils::ls(std::string dirPath, int maxItemsToList)
{
    ListSettings lise;
    lise.maxItemsToList = maxItemsToList;
    return lsx(dirPath, lise);
}
std::vector<std::string> FsUtils::ls(std::string dirPath)
{
    return ls(dirPath, 1024);
}

std::vector<std::string> FsUtils::getDirContents(std::string dirPath, ListSettings& lise, RecursionSettings& rese)
{
    //Result to be returned
    std::vector<std::string> res;

    //Go through all the entries of this directory...
    std::vector<std::string> lsRes;
    lsRes = lsx(dirPath, lise);
    for(int i = 0; i<lsRes.size(); i++) {
        std::string entpath;
        if(dirPath.size()>0) {
            entpath = dirPath+"/"+lsRes[i];
        } else {
            entpath = lsRes[i];
        }

        //If doing recursive listing, add everything within any dir we come across.
        std::vector<std::string> subDirContents;
        if(rese.recursiveSearch && dirExists(entpath) && rese.numLayersDown<rese.maxLayersDown) {
            bool shouldSkip = false;
            for(int j = 0; j<rese.skippedDirPaths.size() && !shouldSkip; j++) {
                if(dirPath==rese.skippedDirPaths[j]) { shouldSkip = true; }
            }

            if(!shouldSkip) {
                ListSettings liseNew = lise; liseNew.maxItemsToList = lise.maxItemsToList-res.size();
                RecursionSettings reseNew = rese; reseNew.numLayersDown++;

                subDirContents = getDirContents(entpath, liseNew, reseNew);
            }
        }

        //Add this entry
        if(res.size()<lise.maxItemsToList) {
            res.push_back(entpath);
	    }
        //Add everything in subDirContents.
        for(int i = 0; i<subDirContents.size(); i++) {
            if(res.size()<lise.maxItemsToList) {
                res.push_back(subDirContents[i]);
            }
        }
    }

    return res;
}

std::vector<std::string> FsUtils::getDirContents(std::string dirPath, ListSettings& lise) { RecursionSettings rese; return getDirContents(dirPath, lise, rese); }
std::vector<std::string> FsUtils::getDirContents(std::string dirPath) { ListSettings lise; return getDirContents(dirPath, lise); }

std::vector<std::string> FsUtils::getManyDirContents(std::vector<std::string> dirPaths, ListSettings& lise, RecursionSettings& rese)
{
    std::vector<std::string> res;
    
    //Loop through all dirPaths
    for(int i = 0; i<dirPaths.size(); i++) {
        //Loop through this dirPath's list of strings.
        auto gdc = getDirContents(dirPaths[i], lise, rese);
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
    FilePath pfp(path);
    path = pfp.get();
    
    int i = -1;
    for(i = path.size()-1; i>=0; i--) {
        if(path[i]=='/') { break; }
    }

    std::vector<std::string> dirFileList;
    ListSettings lise;      lise.includeDirs = false;
    RecursionSettings rese; rese.recursiveSearch = false;
    if(i==-1) { dirFileList = getDirContents("", lise, rese); }
    else      { dirFileList = getDirContents(path.substr(0, i), lise, rese);}

    std::string res = "?null?";
    int count = 0;
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
        res = "?null?";
    }

    if(logWarnings && count>1) Log::warnv(__PRETTY_FUNCTION__, "returning "+res, "Found %d possible matches for \"%s\"", count, path.c_str());
    if(res=="?null?") {
        throw std::logic_error("Found no files matching the provided path");
    }
    return res;
}

void FsUtils::setLogWarnings(bool lw) {
    logWarnings = lw;
}

bool FsUtils::tryAddToDirentList(std::string dirPath, std::vector<std::string>& vec, std::string ent, ListSettings& lise)
{
    /* Prelims */
    //If we are about to exceed the maxItemsToList, stop.
    if(vec.size()>=lise.maxItemsToList) return false;
    //If this is the "." or ".." directory, stop.
    if(ent=="." || ent=="..") return false;

    /* Default ls behavior */
    //Simply list dirs and files without checking their type
    if(lise.includeDirs && lise.includeFiles && lise.includeHiddenEntries && !lise.excludeSymlinkDirs) {
        vec.push_back(ent);
        return true;
    }

    /* Custom ls behavior */
    //Build full path of dir entry
    std::string entpath = dirPath+"/"+ent;
    //Exclude hidden entries if necessary
    if(!lise.includeHiddenEntries && ent[0]=='.') {
        return false;
    }

    //If including dirs...
    if(lise.includeDirs && dirExists(entpath)) {
        if(lise.excludeSymlinkDirs) {
            if(!symlinkExists(entpath)) {
                vec.push_back(ent);
                return true;
            }
        } else {
            vec.push_back(ent);
            return true;    
        }
    }
    //If including files...
    if(lise.includeFiles && fileExists(entpath)) {
        vec.push_back(ent);
        return true;
    }

    //If neither...?
    return false;
}
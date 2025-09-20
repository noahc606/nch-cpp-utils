#include "FilePath.h"
#include <exception>
#include <sstream>
#include <stdexcept>
#include <vector>
#include "FsUtils.h"

using namespace nch;

FilePath::FilePath(std::string path)
{
    std::stringstream res;
    for(int i = 0; i<path.size(); i++) {
        if(path[i]=='\"') { continue; }             //Replace backslashes ('\') with forward slashes ('/')
        res << path[i];                             //Normal characters
    }

    cleanpath = res.str();
}

std::string FilePath::get() { return cleanpath; }
std::string FilePath::getObjectName(bool includeExtension)
{
    std::string wp = get(); //Working Path
    if(!includeExtension) {
        wp = getWithoutExtension();
    }

    if(!FsUtils::pathExists(get())) return "?null?";
    
    std::string filename = "";
    for(int i = wp.size()-1; i>=0; i--) {
        if(wp[i]=='/') {
            break;
        } else {
            filename = wp[i]+filename;
        }
    }
    return filename;
}
std::string FilePath::getObjectName() { return getObjectName(true); }
std::string FilePath::getGrandparentDirName(int numUpDirs)
{
    if(numUpDirs<1)
        throw std::exception(std::invalid_argument("numUpDirs must be >=1"));

    std::string grandparentDir = "";
    int slashesFound = 0;
    for(int i = cleanpath.size()-1; i>=0; i--) {
        if(slashesFound<numUpDirs && cleanpath[i]=='/') {
            slashesFound++;
            continue;
        }

        if(slashesFound==numUpDirs) {
            if(cleanpath[i]=='/') {
                slashesFound++;
                if(slashesFound>numUpDirs) {
                    break;
                }
            }
            grandparentDir = cleanpath[i]+grandparentDir;
        }
    }

    if(grandparentDir=="") {
        throw std::exception(std::invalid_argument("Arrived at root directory"));
    }
    return grandparentDir;
}
std::string FilePath::getParentDirName() { return getGrandparentDirName(1); }

std::string FilePath::getParentDirPath()
{
    if(!FsUtils::dirExists(get()) && !FsUtils::fileExists(get())) {
        throw std::exception(std::out_of_range("The file pointed to by this object does not exist"));
    }

    std::string s = "/"+getObjectName(true);
    if(cleanpath.substr(cleanpath.size()-s.size())==s) {
        return cleanpath.substr(0, cleanpath.size()-s.size());
    }
    return cleanpath;
}

int FilePath::getNumDirsDown()
{
    int slashes = 0;
    for(int i = 0; i<cleanpath.size(); i++) {
        if(cleanpath[i]=='/') slashes++;
    }
    return slashes+1;
}

std::string FilePath::getExtension()
{
    if(FsUtils::dirExists(get()))   return "?directory?";
    if(!FsUtils::fileExists(get())) return "?null?";

    std::string ext = "";
    for(int i = cleanpath.size()-1; i>=0; i--) {
        if(cleanpath[i]=='.') {
            break;
        } else if(cleanpath[i]=='/') {
            break;
        } else {
            ext = cleanpath[i]+ext;
        }
    }
    std::string res = ext;
    return res;
}

std::string FilePath::getWithoutExtension()
{
    if(FsUtils::dirExists(get()))   return get();
    if(!FsUtils::fileExists(get())) return get();


    std::string s = "."+getExtension();
    if(cleanpath.substr(cleanpath.size()-s.size())==s) {
        return cleanpath.substr(0, cleanpath.size()-s.size());
    }
    return cleanpath;
}
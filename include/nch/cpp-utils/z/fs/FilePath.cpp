#include "FilePath.h"
#include <bits/stl_algo.h>
#include <exception>
#include <nch/cpp-utils/string-utils.h>
#include <sstream>
#include <stdexcept>
#include <vector>
#include "FsUtils.h"

using namespace nch;

FilePath::FilePath(std::string path)
{
    std::stringstream res;
    char prev = ' ';
    for(int i = 0; i<path.size(); i++) {
        if(path[i]=='\\') path[i] = '/';

        //If this character and previous were slashes, skip.
        if(path[i]=='/' && prev=='/') {
            continue;
        }
        
        res << path[i]; //Normal characters
        prev = path[i];
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
    
    int lastIdx = wp.size()-1;
    for(int i = wp.size()-1; i>=0; i--) {
        if(wp[i]!='/') {
            lastIdx = i;
            break;
        }

    }

    std::string filename = "";
    for(int i = lastIdx; i>=0; i--) {
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
    if(cleanpath.size()==0) return "";
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
    if(!StringUtils::aContainsB(cleanpath, ".")) return "";

    std::string ret = "";
    for(int i = cleanpath.size()-1; i>=0; i--) {
        if(cleanpath[i]=='.') {
            break;
        } else if(cleanpath[i]=='/') {
            break;
        } else {
            ret = cleanpath[i]+ret;
        }
    }
    std::transform(ret.begin(), ret.end(), ret.begin(), ::tolower);
    return ret;
}

std::string FilePath::getWithoutExtension()
{
    if(cleanpath=="") return "";

    std::string s = "."+getExtension();
    if(cleanpath.substr(cleanpath.size()-s.size())==s) {
        return cleanpath.substr(0, cleanpath.size()-s.size());
    }
    return cleanpath;
}

bool FilePath::isHidden()
{
    try { return getObjectName().at(0)=='.'; } catch(...) {}
    return false;
}
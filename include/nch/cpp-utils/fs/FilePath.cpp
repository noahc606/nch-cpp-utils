#include "FilePath.h"
#include <sstream>
#include <vector>
#include "FsUtils.h"

using namespace nch;
const std::string FilePath::validChars = "AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz0123456789-+= _/\\";

FilePath::FilePath(std::string path)
{
    if(FsUtils::fileExists(path)) {
        //Make sure path for a local file is valid
        std::string vc = validChars+".";
        for(int i = 0; i<path.size(); i++) {
            //If invalid char found, stop
            if(vc.find(path[i])==std::string::npos) {
                printf("Invalid characters within path \"%s\" (Use [Aa-Zz][ _/\\] and make sure a file extension exists).\n", path.c_str());
                cleanpath = "?null?";
                return;
            }
        }
    }

    if(FsUtils::dirExists(path)) {
        //Make sure path for a local directory is valid
        std::string vc = validChars;
        for(int i = 0; i<path.size(); i++) {
            //If invalid char found, stop
            if(vc.find(path[i])==std::string::npos) {
                printf("Invalid characters within directory path \"%s\" (Use [Aa-Zz][ _/\\]).\n", path.c_str());
                cleanpath = "?null?";
                return;
            }
        }
    }

    std::stringstream res;
    for(int i = 0; i<path.size(); i++) {
        if(path[i]=='\"') { continue; }             //Replace backslashes ('\') with forward slashes ('/')
        res << path[i];                             //Normal characters
    }

    cleanpath = res.str();
}

std::string FilePath::get() { return cleanpath; }
std::string FilePath::getFilename(bool includeExtension)
{
    std::string wp = get(); //Working Path
    if(!includeExtension) {
        wp = getWithoutExtension();
    }

    if(FsUtils::dirExists(get()))   return "?directory?";
    if(!FsUtils::fileExists(get())) return "?null?";
    
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
std::string FilePath::getFilename() { return getFilename(true); }
std::string FilePath::getGrandparentDir(int numUpDirs)
{
    if(numUpDirs<1) return "?invalid-numUpDirs?";

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
        return "?root?";
    }
    return grandparentDir;
}
std::string FilePath::getParentDir() { return getGrandparentDir(1); }

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


    std::string s = getExtension();
    int i = cleanpath.find(s);
    if(i!=std::string::npos) {
        return cleanpath.substr(0, i-1);
    } else {
        printf("Error: Could not get file extension of file \"%s\"\n", get().c_str());
        return "?null?";
    }
}
#include "nch/json-utils/json.h"
#include <fstream>
#include "nch/cpp-utils/fs-utils.h"
using namespace nch;

nlohmann::json JSON::loadFromFile(const std::string& path)
{
    nlohmann::json ret;
    if(!FsUtils::fileExists(path)) {
        Log::warnv(__PRETTY_FUNCTION__, "returning empty JSON object." "File @ \"%s\" doesn't exist", path.c_str());
        return ret;
    }

    try {
        std::ifstream ifs(path);
        if(ifs.fail()) throw std::exception();
        ret = nlohmann::json::parse(ifs);
    } catch(...) {
        Log::error(__PRETTY_FUNCTION__, "Failed to parse JSON from file @ \"%s\"", path.c_str());
        return ret;
    }

    return ret;
}
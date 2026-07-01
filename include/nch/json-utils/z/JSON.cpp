#include "nch/json-utils/json.h"
#include <fstream>
#include "nch/cpp-utils/fs-utils.h"
#include "nch/cpp-utils/string-utils.h"
using namespace nch;

nlohmann::json JSON::loadFromFile(const std::string& path)
{
    nlohmann::json ret;
    if(!FsUtils::fileExists(path)) {
        Log::warnv(__PRETTY_FUNCTION__, "returning empty JSON object", "File @ \"%s\" doesn't exist", path.c_str());
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
Vec3f JSON::parseVec3f(const nlohmann::json& jsonArray, const std::string& arrayKey, const std::string& context) {
    if(!jsonArray.is_array() || jsonArray.size()<3) throw std::invalid_argument(nch::cat("Failed to parse '", arrayKey, "' from ", context));
    Vec3f ret;
    try {
        ret.x = jsonArray.at(0).get<float>();
        ret.y = jsonArray.at(1).get<float>();
        ret.z = jsonArray.at(2).get<float>();
    } catch(...) {
        throw std::invalid_argument(nch::cat("Failed to parse '", arrayKey, "' from ", context, " as a Vec3f"));
    }

    return ret;
}
Vec3i64 JSON::parseVec3i64(const nlohmann::json& jsonArray, const std::string& arrayKey, const std::string& context) {
    if(!jsonArray.is_array() || jsonArray.size()<3) throw std::invalid_argument(nch::cat("Failed to parse '", arrayKey, "' from ", context));
    Vec3i64 ret;
    try {
        ret.x = jsonArray.at(0).get<int64_t>();
        ret.y = jsonArray.at(1).get<int64_t>();
        ret.z = jsonArray.at(2).get<int64_t>();
    } catch(...) {
        throw std::invalid_argument(nch::cat("Failed to parse '", arrayKey, "' from ", context, " as a Vec3f"));
    }

    return ret;
}
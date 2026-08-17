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
bool JSON::saveToFile(const std::string& path, const nlohmann::json& j, int indent)
{
    try {
        std::ofstream ofs(path, std::ios::trunc);
        if(ofs.fail()) throw std::exception();
        ofs << j.dump(indent);
        ofs.close();
    } catch(...) {
        Log::error(__PRETTY_FUNCTION__, "Failed to write JSON to file @ \"%s\"", path.c_str());
        return false;
    }

    return true;
}
nlohmann::json JSON::parse(const std::string& text, const std::string& context)
{
    nlohmann::json ret = nlohmann::json::parse(text, nullptr, false);
    if(ret.is_discarded()) {
        if(context!="") {
            Log::error(__PRETTY_FUNCTION__, "Failed to parse JSON from %s: \"%s\"", context.c_str(), text.c_str());
        } else {
            Log::error(__PRETTY_FUNCTION__, "Failed to parse JSON from \"%s\"", text.c_str());
        }
        return nlohmann::json();
    }

    return ret;
}

bool JSON::has(const nlohmann::json& j, const std::string& key)
{
    if(!j.is_object()) return false;
    auto itr = j.find(key);
    return itr!=j.end() && !itr->is_null();
}
const nlohmann::json& JSON::getObject(const nlohmann::json& j, const std::string& key)
{
    static const nlohmann::json emptyObject = nlohmann::json::object();
    if(!has(j, key)) return emptyObject;

    const nlohmann::json& ret = j.at(key);
    if(!ret.is_object()) return emptyObject;
    return ret;
}
const nlohmann::json& JSON::getArray(const nlohmann::json& j, const std::string& key)
{
    static const nlohmann::json emptyArray = nlohmann::json::array();
    if(!has(j, key)) return emptyArray;

    const nlohmann::json& ret = j.at(key);
    if(!ret.is_array()) return emptyArray;
    return ret;
}
std::string JSON::getOpt(const nlohmann::json& j, const std::string& key, const char* fallback)
{
    return getOpt<std::string>(j, key, std::string(fallback));
}

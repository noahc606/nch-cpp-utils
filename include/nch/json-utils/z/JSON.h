#pragma once
#include <nlohmann/json.hpp>
#include "nch/cpp-utils/log.h"
namespace nch { class JSON {
public:
    static nlohmann::json loadFromFile(const std::string& path);
    template<typename T> static bool loadFromOptionalField(T& var, const nlohmann::json& jsonData, const std::string& jsonKey, const std::string& jsonFilePath = "")
    {
        try {
            var = jsonData.at(jsonKey);
            return true;
        }
        catch (const nlohmann::json::out_of_range& e) {}
        catch (const std::exception& e) {
            if(jsonFilePath!="") {
                nch::Log::errorv(__PRETTY_FUNCTION__, e.what(), "Unexpected error while reading \"%s\" from file \"%s\": ", jsonKey.c_str(), jsonFilePath.c_str());
            } else {
                nch::Log::errorv(__PRETTY_FUNCTION__, e.what(), "Unexpected error while reading \"%s\" from json \"%s\"", jsonKey.c_str(), jsonData.dump().c_str());
            }
        }

        return false;
    }
private:
}; }
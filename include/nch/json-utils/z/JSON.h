#pragma once
#include <nlohmann/json.hpp>
#include "nch/cpp-utils/log.h"
namespace nch { class JSON {
public:
    static nlohmann::json loadFromFile(const std::string& path);

    template<typename T> static void loadFromRequiredField(T& var, const nlohmann::json& jsonData, const std::string& jsonKey, const std::string& jsonFilePath = "")
    {
        try {
            var = jsonData.at(jsonKey);
        }
        catch (const std::exception& e) {
            if(jsonFilePath!="") {
                throw std::invalid_argument(nch::Log::getFormattedString("Missing required key \"%s\" from file \"%s\": %s", jsonKey.c_str(), jsonFilePath.c_str(), e.what()));
            } else {
                throw std::invalid_argument(nch::Log::getFormattedString("Missing required key \"%s\" from json \"%s\": %s", jsonKey.c_str(), jsonData.dump().c_str(), e.what()));
            }
        }
    }
    template<typename T> static bool loadFromOptionalField(T& var, const nlohmann::json& jsonData, const std::string& jsonKey, const std::string& jsonFilePath = "")
    {
        try {
            var = jsonData.at(jsonKey);
            return true;
        }
        catch (const nlohmann::json::out_of_range& e) {}
        catch (const std::exception& e) {
            if(jsonFilePath!="") {
                nch::Log::error(__PRETTY_FUNCTION__, "Unexpected error while reading \"%s\" from file \"%s\": %s", jsonKey.c_str(), jsonFilePath.c_str(), e.what());
            } else {
                nch::Log::error(__PRETTY_FUNCTION__, "Unexpected error while reading \"%s\" from json \"%s\": %s", jsonKey.c_str(), jsonData.dump().c_str(), e.what());
            }
        }

        return false;
    }
private:
}; }
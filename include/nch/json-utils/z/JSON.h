#pragma once
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>
#include "nch/cpp-utils/log.h"
#include "nch/math-utils/vec2.h"
#include "nch/math-utils/vec3.h"
#include "nch/math-utils/vec4.h"
namespace nch {

/* An nch vector is a JSON array of its components. ADL finds these, so nlohmann treats a Vec like any
   built-in type - "j[k] = pos;", "pos = j.at(k).get<Vec3d>();" - and every reader below (getOpt, getList,
   loadFromOptionalField, ...) takes one with no separate parsing step. */
template<typename T> void to_json(nlohmann::json& j, const Vec2<T>& v) { j = nlohmann::json::array({v.x, v.y}); }
template<typename T> void to_json(nlohmann::json& j, const Vec3<T>& v) { j = nlohmann::json::array({v.x, v.y, v.z}); }
template<typename T> void to_json(nlohmann::json& j, const Vec4<T>& v) { j = nlohmann::json::array({v[0], v[1], v[2], v[3]}); }
template<typename T> void from_json(const nlohmann::json& j, Vec2<T>& v)
{
    if(!j.is_array() || j.size()<2) throw std::invalid_argument("Expected an [x, y] array, got "+j.dump());
    v.x = j.at(0).get<T>();
    v.y = j.at(1).get<T>();
}
template<typename T> void from_json(const nlohmann::json& j, Vec3<T>& v)
{
    if(!j.is_array() || j.size()<3) throw std::invalid_argument("Expected an [x, y, z] array, got "+j.dump());
    v.x = j.at(0).get<T>();
    v.y = j.at(1).get<T>();
    v.z = j.at(2).get<T>();
}
template<typename T> void from_json(const nlohmann::json& j, Vec4<T>& v)
{
    if(!j.is_array() || j.size()<4) throw std::invalid_argument("Expected an [x, y, z, w] array, got "+j.dump());
    for(int i = 0; i<4; i++) v[i] = j.at(i).get<T>();
}

class JSON {
public:
    /* Whole files */
    //Parse a file into JSON. Never throws: a missing or malformed file logs and yields an empty object.
    static nlohmann::json loadFromFile(const std::string& path);
    /**
     * @brief Write JSON out to a file, creating or truncating it. Never throws.
     * @param indent Spaces per level, or -1 for the compact single-line form.
     * @return Whether the file was written.
     */
    static bool saveToFile(const std::string& path, const nlohmann::json& j, int indent = 4);
    /**
     * @brief Parse raw text into JSON. Never throws, unlike nlohmann's own parse.
     * @param context Named in the log message when the text doesn't parse (a table row, a lua return, ...).
     * @return The parsed value, or a null JSON if the text was malformed. Test the result with is_null().
     */
    static nlohmann::json parse(const std::string& text, const std::string& context = "");

    /* Sub-values kept in their JSON form */
    //True when 'j' is an object holding a non-null value at 'key'. False for any non-object, so it's safe on anything.
    static bool has(const nlohmann::json& j, const std::string& key);
    //The object at 'key', or a shared empty object when it's missing or isn't one - so ".items()" is always safe to walk.
    static const nlohmann::json& getObject(const nlohmann::json& j, const std::string& key);
    //The array at 'key', or a shared empty array when it's missing or isn't one - so a range-for is always safe.
    static const nlohmann::json& getArray(const nlohmann::json& j, const std::string& key);

    /* Values */
    /**
     * @brief Read a key that a file can't be valid without.
     * @param context File path or asset name to name in the error, if the key is missing or the wrong type.
     * @throws std::invalid_argument Always, when the key can't be read as a T.
     */
    template<typename T> static T getReq(const nlohmann::json& j, const std::string& key, const std::string& context = "")
    {
        try {
            return j.at(key).get<T>();
        } catch(const std::exception& e) {
            if(context!="") {
                throw std::invalid_argument(Log::getFormattedString("Missing or malformed required key \"%s\" from \"%s\": %s", key.c_str(), context.c_str(), e.what()));
            }
            throw std::invalid_argument(Log::getFormattedString("Missing or malformed required key \"%s\" from json \"%s\": %s", key.c_str(), j.dump().c_str(), e.what()));
        }
    }
    /**
     * @brief Read a lone value - one already pulled out of its object, or passed in as an argument - as a T.
     * @param context What the value is, named in the error thrown when it can't be read as one.
     * @throws std::invalid_argument When the value isn't a T.
     */
    template<typename T> static T toValue(const nlohmann::json& jv, const std::string& context = "")
    {
        try {
            return jv.get<T>();
        } catch(const std::exception& e) {
            if(context!="") {
                throw std::invalid_argument(Log::getFormattedString("Failed to read %s: %s", context.c_str(), e.what()));
            }
            throw std::invalid_argument(Log::getFormattedString("Failed to read json \"%s\": %s", jv.dump().c_str(), e.what()));
        }
    }
    //Read an optional key, falling back to 'fallback' when it's absent, null, or unreadable as a T (which is logged).
    template<typename T> static T getOpt(const nlohmann::json& j, const std::string& key, const T& fallback)
    {
        if(!has(j, key)) return fallback;
        try {
            return j.at(key).get<T>();
        } catch(const std::exception& e) {
            Log::warnv(__PRETTY_FUNCTION__, "using the fallback", "Couldn't read \"%s\" from json \"%s\": %s", key.c_str(), j.dump().c_str(), e.what());
        }
        return fallback;
    }
    //Overload so a string literal fallback yields a std::string rather than deducing a char array.
    static std::string getOpt(const nlohmann::json& j, const std::string& key, const char* fallback);

    /* Lists: a lone value counts as a list of one, which is how "one or many" data keys are written */
    //An absent key is an empty list; a malformed element throws.
    template<typename T> static std::vector<T> getList(const nlohmann::json& j, const std::string& key)
    {
        if(!has(j, key)) return std::vector<T>();
        return toList<T>(j.at(key));
    }
    template<typename T> static std::vector<T> toList(const nlohmann::json& jv)
    {
        std::vector<T> ret;
        if(jv.is_null()) return ret;
        if(!jv.is_array()) {
            ret.push_back(jv.get<T>());
            return ret;
        }

        ret.reserve(jv.size());
        for(const nlohmann::json& e : jv) ret.push_back(e.get<T>());
        return ret;
    }
    /**
     * @brief Read a "[min, max]" pair, or a lone value standing for both ends of it.
     * @return Whether the key was present. Both outs keep their old values when it wasn't.
     * @throws std::invalid_argument When the key is an array of any length but 2.
     */
    template<typename T> static bool readRange(T& outMin, T& outMax, const nlohmann::json& j, const std::string& key)
    {
        if(!has(j, key)) return false;

        const nlohmann::json& v = j.at(key);
        if(!v.is_array()) {
            outMin = outMax = v.get<T>();
            return true;
        }
        if(v.size()!=2) throw std::invalid_argument(Log::getFormattedString("Key \"%s\" must be a [min, max] pair or a lone value, got \"%s\"", key.c_str(), v.dump().c_str()));
        outMin = v.at(0).get<T>();
        outMax = v.at(1).get<T>();
        return true;
    }

    /* In-place reads, for when the variable already holds the default */
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

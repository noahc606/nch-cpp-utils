#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace nch { class StringUtils {
public:
    static std::vector<std::string> split(std::string toSplit, char delim);
    static std::vector<int64_t> parseI64Array(std::string s);
    static std::vector<int64_t> parseI64ArraySimple(std::string s);
    static std::string extractBracketedStr(std::string s);
    static std::string trimmed(std::string s);
    static std::string stringFromBytestream(const std::vector<unsigned char>& byteStream, bool keepZeros);
    static std::string stringFromBytestream(const std::vector<unsigned char>& byteStream);
    static std::vector<unsigned char> bytestreamFromString(const std::string& str);
    static bool aHasPrefixB(const std::string& a, const std::string& b);
    static bool aHasSuffixB(const std::string& a, const std::string& b);
    static bool aContainsB(const std::string& a, const std::string& b);
    static bool aContainsAllMembersOfB(const std::string& a, const std::vector<std::string>& b);

    static int parseCmdArg(const std::vector<std::string>& args, std::string argLabel, int defaultValue, int errorValue);
    static std::string parseCmdArg(const std::vector<std::string>& args, std::string argLabel, std::string defaultValue, std::string errorValue);
    static bool cmdArgExists(const std::vector<std::string>& args, std::string arg);

    static bool validateString(std::string s, std::string charSet);
    static bool validateAlphanumeric(std::string s);
    static bool validateInjectionless(std::string s);
    static bool validateSpaceless(std::string s);
    static bool validateCmdArgLabel(std::string s);
    static bool validateSafeString(std::string s);
    static bool validateIP(std::string s);
private:

}; }
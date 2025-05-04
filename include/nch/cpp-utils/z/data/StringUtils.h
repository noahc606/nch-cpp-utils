#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace nch { class StringUtils {
public:
    static std::vector<std::string> split(std::string toSplit, char delim);
    static std::vector<int64_t> parseI64Array(std::string s);
    static std::string extractBracketedStr(std::string s);
    static std::string trimmed(std::string s);
    static std::string fromByteStream(std::vector<char>& byteStream);
    static bool aHasPrefixB(std::string a, std::string b);
    static bool aHasSuffixB(std::string a, std::string b);
    static bool validateString(std::string s, std::string charSet);
    static bool validateAlphanumeric(std::string s);
    static bool validateInjectionless(std::string s);
    static bool validateSafeString(std::string s);
private:

}; }
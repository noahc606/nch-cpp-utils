#pragma once
#include <vector>
#include <string>

namespace nch { class StringUtils {
public:
    static std::vector<std::string> split(std::string toSplit, char delim);
    static std::vector<int64_t> parseI64Array(std::string s);
    static std::string extractBracketedStr(std::string s);

private:

}; }
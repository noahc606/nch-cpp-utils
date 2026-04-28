#pragma once
#include <string>
class SQL_Utils {
public:
    static std::string hashedToBase64(const std::string& input);
    static std::string replacedUnicodeTags(const std::string& input);
private:    
    static std::string codepointToUTF8(unsigned int cp);
};
#pragma once
#include <string>

namespace nch { class FileUtils {
public:
    static void writeToFile(FILE* pFile, std::string str);
    static void writeToFile(FILE* pFile, unsigned char c);
private:
};}
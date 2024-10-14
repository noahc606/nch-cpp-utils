#pragma once
#include <string>
#include <vector>

namespace nch { class FileUtils {
public:
    static void writeToFile(FILE* pFile, std::string str);
    static void writeToFile(FILE* pFile, unsigned char c);
    static std::vector<std::string> getFileLines(FILE* pFile);
private:
};}
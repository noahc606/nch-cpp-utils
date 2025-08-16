#pragma once
#include <string>
#include <vector>

namespace nch { class FileUtils {
public:
    static void writeToFile(FILE* pFile, std::string str);
    static void writeToFile(FILE* pFile, unsigned char c);
    static std::string getFileContent(FILE* pFile);
    static std::string readFileContent(std::string path);
    static std::vector<unsigned char> getFileBytes(FILE* pFile);
    static std::vector<unsigned char> readFileBytes(std::string path);
    static std::vector<std::string> getFileLines(FILE* pFile);
    static std::vector<std::string> readFileLines(std::string path);
private:
};}
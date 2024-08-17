#pragma once
#include <string>

class NCH_FileUtils {
public:
    static void writeToFile(FILE* pFile, std::string str);
    static void writeToFile(FILE* pFile, unsigned char c);
private:
};
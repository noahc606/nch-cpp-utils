#include "FileUtils.h"

using namespace nch;

void FileUtils::writeToFile(FILE* pFile, std::string str)
{
    size_t size = sizeof(unsigned char)*str.size();         //Get size (in bytes) of the whole string
    unsigned char* buffer = (unsigned char*)malloc(size);   //Get an (unsigned char*) buffer
    
    //Populate buffer
    for(int i = 0; i<str.size(); i++) {
        buffer[i] = str[i];
    }

    //Write buffer to file then delete the malloc'ed buffer
    fwrite(buffer, size, 1, pFile);
    delete[] buffer;
}

void FileUtils::writeToFile(FILE* pFile, unsigned char c)
{
    unsigned char* buffer = (unsigned char*)malloc(sizeof(unsigned char*));
    buffer[0] = c;
    
    //Write buffer to file then delete the malloc'ed buffer
    fwrite(buffer, 1, 1, pFile);
    delete[] buffer;
}
#include "FileUtils.h"
#include <sstream>

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

std::vector<std::string> FileUtils::getFileLines(FILE* pFile)
{
    std::vector<std::string> res;
    bool foundNewLine = false;
    std::stringstream currentLine;
	
	char c = '\0';
	while( (c = std::fgetc(pFile))!=EOF ) {
        if( !foundNewLine && (c=='\n' || c=='\r') ) {  /* Found newline character */
            foundNewLine = true;

            res.push_back(currentLine.str());
            currentLine.str(std::string());
            currentLine.clear();
        } else {                                        /* Found any other character */
            foundNewLine = false;
            currentLine << c;
        }
    }

    //Add last currentLine
    res.push_back(currentLine.str());

    //Return result
    return res;
}
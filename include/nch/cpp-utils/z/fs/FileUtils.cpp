#include "FileUtils.h"
#include "nch/cpp-utils/log.h"
#include <sstream>

using namespace nch;

/*
    Write a string of arbitrary length to a file.
    
    Note: does not check if pFile==NULL.
*/
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

/*
    Recommended to be used to write a byte to a file.

    Note: Does not check if pFile==NULL.
*/
void FileUtils::writeToFile(FILE* pFile, unsigned char c)
{
    unsigned char* buffer = (unsigned char*)malloc(sizeof(unsigned char*));
    buffer[0] = c;
    
    //Write buffer to file then delete the malloc'ed buffer
    fwrite(buffer, 1, 1, pFile);
    delete[] buffer;
}

/*
    Returns: the content of an entire file as a single string.
*/
std::string FileUtils::getFileContent(FILE* pFile)
{
    if(pFile==NULL) {
        Log::error(__PRETTY_FUNCTION__, "pFile is null (returned empty string \"\")");
        return "";
    }

    std::stringstream ret;
	char c = '\0';
	while( (c = std::fgetc(pFile))!=EOF ) {
        ret << c;
    }
    return ret.str();
}
std::string FileUtils::readFileContent(std::string path)
{
    FILE* pFile = fopen(path.c_str(), "r");    
    std::string ret = FileUtils::getFileContent(pFile);
    fclose(pFile);
    return ret;
}

std::vector<unsigned char> FileUtils::getFileBytes(FILE* pFile)
{
    //Find file size
    long fileSize; {
        fseek(pFile, 0, SEEK_END);
        fileSize = ftell(pFile);
        fseek(pFile, 0, 0);    
    }

    //Build the 'ret'urn object
    std::vector<unsigned char> ret;
    ret.reserve(fileSize*sizeof(unsigned char));
    for(int i = 0; i<fileSize; i++) {
        ret[i] = fgetc(pFile);
    }
    return ret;
}

std::vector<unsigned char> FileUtils::readFileBytes(std::string path)
{
    //Open file to be encrypted (read+binary)
    FILE* pFile = fopen(path.c_str(), "rb");
    auto ret = getFileBytes(pFile);
    fclose(pFile);
    return ret;
}

/*
    Returns a list of the lines within a file. A line break is defined as any occurrence of '\n' or '\r'. There is no trimming or processing of the lines, including empty lines.

    Returns: A vector<string> of each line of an entire file.
*/
std::vector<std::string> FileUtils::getFileLines(FILE* pFile, bool includeEmptyLines)
{
    std::vector<std::string> ret;
    if(pFile==NULL) {
        Log::error(__PRETTY_FUNCTION__, "pFile is null (returned empty vector)");
        return ret;
    }

    bool foundNewLine = false;
    std::stringstream currentLine;
	
	char c = '\0';
	while( (c = std::fgetc(pFile))!=EOF ) {
        if( !foundNewLine && (c=='\n' || c=='\r') ) {  /* Found newline character */
            foundNewLine = true;

            if(includeEmptyLines || currentLine.str().size()>0) {
                ret.push_back(currentLine.str());
            }
            currentLine.str(std::string());
            currentLine.clear();
        } else {                                        /* Found any other character */
            foundNewLine = false;
            currentLine << c;
        }
    }

    //Add last currentLine
    if(includeEmptyLines || currentLine.str().size()>0) {
        ret.push_back(currentLine.str());
    }

    //Return
    return ret;
}

std::vector<std::string> FileUtils::readFileLines(std::string path, bool includeEmptyLines)
{
    FILE* pFile = fopen(path.c_str(), "r");
    std::vector<std::string> ret = FileUtils::getFileLines(pFile);
    fclose(pFile);
    return ret;
}

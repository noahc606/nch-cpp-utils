#include "NoahSimpleCryption.h"
#include "nch/cpp-utils/fs-utils.h"
#include "nch/cpp-utils/file-utils.h"

using namespace nch;

void NoahSimpleCryption::encryptFile(std::string file, std::string key)
{
    encryptFile(file, getShiftSetFromStr(key));
}

void NoahSimpleCryption::decryptFile(std::string file, std::string key)
{
    encryptFile(file, getShiftSetComplement( getShiftSetFromStr(key) ));
}




std::vector<unsigned char> NoahSimpleCryption::getShiftSetComplement(std::vector<unsigned char> shiftset)
{
    std::vector<unsigned char> res;
    for(int i = 0; i<shiftset.size();i++) {
        res.push_back(256-shiftset[i]);
    }

    return res;
}

std::vector<unsigned char> NoahSimpleCryption::getShiftSetFromStr(std::string str)
{
    std::vector<unsigned char> res;
    for(int i = 0; i<str.size(); i++) {
        res.push_back(str[i]);
    }

    if(str.size()==0) {
        return getShiftSetFromStr("testing passkey 1234 abcd $fk34#$234fRF$");
    }

    return res;
}

void NoahSimpleCryption::encryptFile(std::string file, std::vector<unsigned char> shiftset)
{
    //Open file to be encrypted (read+binary)
    FILE* pFile = fopen(file.c_str(), "rb");

    //Find file size
    fseek(pFile, 0, SEEK_END);
    long fileSize = ftell(pFile);
    fseek(pFile, 0, 0);

    //Build char* buffer which has the contents of the entire file
    unsigned char* buffer = (unsigned char*)malloc(fileSize*sizeof(unsigned char));   //Get an (unsigned char*) buffer
    for(int i = 0; i<fileSize; i++) {
        buffer[i] = fgetc(pFile);
    }
    fclose(pFile);

    //Re-open file (write+binary)
    fopen(file.c_str(), "wb");

    //Rewrite file where every character is shifted by a certain amount depending on the 'shiftset'.
    for(int i = 0; i<fileSize; i++) {
        unsigned char c = buffer[i]+(unsigned char)shiftset[(i%shiftset.size())];
        FileUtils::writeToFile(pFile, c);
    }
    fclose(pFile);

    //Cleanup malloc'ed buffer
    delete[] buffer;
}



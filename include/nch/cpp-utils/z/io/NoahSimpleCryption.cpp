#include "NoahSimpleCryption.h"
#include "nch/cpp-utils/fs-utils.h"
#include "nch/cpp-utils/file-utils.h"

using namespace nch;

void NoahSimpleCryption::encryptFile(std::string path, std::string key) {
    shiftFile(path, getShiftSetFromStr(key));
}
void NoahSimpleCryption::decryptFile(std::string path, std::string key) {
    shiftFile(path, getShiftSetComplement(getShiftSetFromStr(key)));
}
void NoahSimpleCryption::encryptBytestream(std::vector<unsigned char>& bytestream, std::string key) {
    shiftBytestream(bytestream, getShiftSetFromStr(key));
}
void NoahSimpleCryption::decryptBytestream(std::vector<unsigned char>& bytestream, std::string key) {
    shiftBytestream(bytestream, getShiftSetComplement(getShiftSetFromStr(key)));
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
void NoahSimpleCryption::shiftFile(std::string path, const std::vector<unsigned char>& shiftset)
{
    //Build char buffer which has the entire binary contents of file
    auto buffer = FileUtils::readFileBytes(path);

    //Rewrite file where every character is shifted by a certain amount depending on the 'shiftset'.
    FILE* pFile = fopen(path.c_str(), "wb");
    for(int i = 0; i<buffer.size(); i++) {
        unsigned char c = buffer[i]+(unsigned char)shiftset[(i%shiftset.size())];
        FileUtils::writeToFile(pFile, c);
    }
    fclose(pFile);
}
void NoahSimpleCryption::shiftBytestream(std::vector<unsigned char>& bytestream, const std::vector<unsigned char>& shiftset)
{
    //Rewrite bytestream where every character is shifted by a certain amount depending on the 'shiftset'.
    for(int i = 0; i<bytestream.size(); i++) {
        bytestream[i] = bytestream[i]+(unsigned char)shiftset[(i%shiftset.size())];
    }
}
#pragma once
#include <string>
#include <vector>

namespace nch { class NoahSimpleCryption
{
public:
    static void encryptFile(std::string path, std::string key);
    static void decryptFile(std::string path, std::string key);
    static void encryptBytestream(std::vector<unsigned char>& bytestream, std::string key);
    static void decryptBytestream(std::vector<unsigned char>& bytestream, std::string key);

private:
    static std::vector<unsigned char> getShiftSetComplement(std::vector<unsigned char> shiftset);
    static std::vector<unsigned char> getShiftSetFromStr(std::string str);

    static void shiftFile(std::string path, const std::vector<unsigned char>& shiftset);
    static void shiftBytestream(std::vector<unsigned char>& bytestream, const std::vector<unsigned char>& shiftset);
};
}

#pragma once
#include <string>
#include <vector>

class NCH_SimpleEncryption {
public:
    void encryptFile(std::string file, std::string key);
    void decryptFile(std::string file, std::string key);
private:
    std::vector<unsigned char> getShiftSetComplement(std::vector<unsigned char> shiftset);
    std::vector<unsigned char> getShiftSetFromStr(std::string str);

    void encryptFile(std::string file, std::vector<unsigned char> shiftset);
}; 

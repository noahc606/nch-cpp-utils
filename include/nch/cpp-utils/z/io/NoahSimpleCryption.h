#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace nch { class NoahSimpleCryption
{
public:
    //Output layout: magic(4) + salt(16) + ciphertext + tag(8). Every call generates a fresh salt,
    //so encrypting identical data twice never produces identical output.
    static void encryptFile(std::string path, std::string key);
    static void encryptBytestream(std::vector<unsigned char>& bytestream, std::string key);

    //False if the data isn't in this format, was modified, or the key is wrong. The input is left
    //untouched when that happens, so a failed decrypt can't be mistaken for empty content.
    static bool decryptFile(std::string path, std::string key);
    static bool decryptBytestream(std::vector<unsigned char>& bytestream, std::string key);

private:
    static uint64_t mix64(uint64_t x);
    static void deriveSubkeys(const std::string& key, const unsigned char* salt, uint64_t& streamSeed, uint64_t& macKey);
    static void applyKeystream(std::vector<unsigned char>& bytestream, size_t start, size_t len, uint64_t streamSeed);
    static uint64_t getMAC(uint64_t macKey, const std::vector<unsigned char>& bytestream, size_t len);
    static std::vector<unsigned char> genSalt();
    static void writeBytes(const std::string& path, const std::vector<unsigned char>& bytestream);
};}

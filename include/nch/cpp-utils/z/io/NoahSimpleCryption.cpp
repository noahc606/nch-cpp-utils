#include "NoahSimpleCryption.h"
#include <random>
#include "nch/cpp-utils/file-utils.h"
#include "nch/cpp-utils/log.h"

using namespace nch;

static const char* nscMagic = "NSC2";
static const size_t nscMagicSize = 4;
static const size_t nscSaltSize = 16;
static const size_t nscTagSize = 8;
static const size_t nscHeaderSize = nscMagicSize+nscSaltSize;
//Sequential mixing rounds applied to the key. Each round feeds the next, so one guess can't be
//spread across cores - this is what stands between a short key (a PIN) and instant offline guessing.
static const uint64_t nscKDF_Iterations = 1<<23;
static const std::string nscDefaultKey = "testing passkey 1234 abcd $fk34#$234fRF$";

void NoahSimpleCryption::encryptFile(std::string path, std::string key)
{
    std::vector<unsigned char> bytestream = FileUtils::readFileBytes(path);
    encryptBytestream(bytestream, key);
    writeBytes(path, bytestream);
}
bool NoahSimpleCryption::decryptFile(std::string path, std::string key)
{
    std::vector<unsigned char> bytestream = FileUtils::readFileBytes(path);
    if(!decryptBytestream(bytestream, key)) return false;
    writeBytes(path, bytestream);
    return true;
}

void NoahSimpleCryption::encryptBytestream(std::vector<unsigned char>& bytestream, std::string key)
{
    std::vector<unsigned char> salt = genSalt();
    uint64_t streamSeed = 0, macKey = 0;
    deriveSubkeys(key, salt.data(), streamSeed, macKey);

    std::vector<unsigned char> res;
    res.reserve(nscHeaderSize+bytestream.size()+nscTagSize);
    for(size_t i = 0; i<nscMagicSize; i++) { res.push_back((unsigned char)nscMagic[i]); }
    for(size_t i = 0; i<nscSaltSize; i++) { res.push_back(salt[i]); }
    res.insert(res.end(), bytestream.begin(), bytestream.end());
    applyKeystream(res, nscHeaderSize, bytestream.size(), streamSeed);

    //Encrypt-then-MAC: the tag covers the ciphertext and the salt it was derived from, so tampering
    //with either is caught before anything gets decrypted.
    uint64_t tag = getMAC(macKey, res, res.size());
    for(size_t i = 0; i<nscTagSize; i++) { res.push_back((unsigned char)((tag>>(8*i))&0xFF)); }

    bytestream = res;
}
bool NoahSimpleCryption::decryptBytestream(std::vector<unsigned char>& bytestream, std::string key)
{
    if(bytestream.size()<nscHeaderSize+nscTagSize) {
        Log::warn(__PRETTY_FUNCTION__, "Data is too small to be NoahSimpleCryption output");
        return false;
    }
    for(size_t i = 0; i<nscMagicSize; i++) {
        if(bytestream[i]!=(unsigned char)nscMagic[i]) {
            Log::warn(__PRETTY_FUNCTION__, "Data is not NoahSimpleCryption output (bad magic)");
            return false;
        }
    }

    uint64_t streamSeed = 0, macKey = 0;
    deriveSubkeys(key, &bytestream[nscMagicSize], streamSeed, macKey);

    size_t bodyEnd = bytestream.size()-nscTagSize;
    uint64_t foundTag = 0;
    for(size_t i = 0; i<nscTagSize; i++) {
        foundTag |= ((uint64_t)bytestream[bodyEnd+i])<<(8*i);
    }
    if(getMAC(macKey, bytestream, bodyEnd)!=foundTag) {
        Log::warn(__PRETTY_FUNCTION__, "Authentication failed - wrong key, or the data was modified");
        return false;
    }

    std::vector<unsigned char> res(bytestream.begin()+nscHeaderSize, bytestream.begin()+bodyEnd);
    applyKeystream(res, 0, res.size(), streamSeed);
    bytestream = res;
    return true;
}

uint64_t NoahSimpleCryption::mix64(uint64_t x)
{
    x += 0x9E3779B97F4A7C15ULL;
    x = (x^(x>>30))*0xBF58476D1CE4E5B9ULL;
    x = (x^(x>>27))*0x94D049BB133111EBULL;
    return x^(x>>31);
}

void NoahSimpleCryption::deriveSubkeys(const std::string& key, const unsigned char* salt, uint64_t& streamSeed, uint64_t& macKey)
{
    std::string workingKey = key;
    if(workingKey.size()==0) workingKey = nscDefaultKey;

    uint64_t h = 0xCBF29CE484222325ULL;
    for(size_t i = 0; i<workingKey.size(); i++) { h = mix64(h^(unsigned char)workingKey[i]); }
    for(size_t i = 0; i<nscSaltSize; i++) { h = mix64(h^salt[i]); }

    for(uint64_t i = 0; i<nscKDF_Iterations; i++) { h = mix64(h^i); }

    //Separate subkeys so the keystream never reveals anything usable about the tag, or the reverse.
    streamSeed = mix64(h^0x5EED5EED5EED5EEDULL);
    macKey = mix64(h^0x1234ABCD1234ABCDULL);
}

void NoahSimpleCryption::applyKeystream(std::vector<unsigned char>& bytestream, size_t start, size_t len, uint64_t streamSeed)
{
    //XOR against a keystream whose every 8-byte block depends on the one before it. The old
    //cipher repeated its key every keyLen bytes, which is what made it readable off long files.
    uint64_t state = streamSeed;
    for(size_t i = 0; i<len; i += 8) {
        state = mix64(state);
        uint64_t block = state;
        for(size_t j = 0; j<8 && i+j<len; j++) {
            bytestream[start+i+j] ^= (unsigned char)(block&0xFF);
            block >>= 8;
        }
    }
}

uint64_t NoahSimpleCryption::getMAC(uint64_t macKey, const std::vector<unsigned char>& bytestream, size_t len)
{
    uint64_t h = mix64(macKey);
    for(size_t i = 0; i<len; i += 8) {
        uint64_t block = 0;
        for(size_t j = 0; j<8 && i+j<len; j++) {
            block |= ((uint64_t)bytestream[i+j])<<(8*j);
        }
        h = mix64(h^block);
    }

    //Fold in the length and the key again so trailing bytes can't be added or dropped unnoticed.
    h = mix64(h^(uint64_t)len);
    return mix64(h^macKey);
}

std::vector<unsigned char> NoahSimpleCryption::genSalt()
{
    std::random_device rd;
    std::vector<unsigned char> salt(nscSaltSize, 0);
    for(size_t i = 0; i<nscSaltSize; i++) {
        salt[i] = (unsigned char)(rd()&0xFF);
    }
    return salt;
}

void NoahSimpleCryption::writeBytes(const std::string& path, const std::vector<unsigned char>& bytestream)
{
    FILE* pFile = fopen(path.c_str(), "wb");
    if(pFile==nullptr) {
        Log::error(__PRETTY_FUNCTION__, "Failed to open \"%s\" for writing", path.c_str());
        return;
    }
    for(size_t i = 0; i<bytestream.size(); i++) {
        FileUtils::writeToFile(pFile, bytestream[i]);
    }
    fclose(pFile);
}

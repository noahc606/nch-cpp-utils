#include "SQL_Utils.h"
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <iomanip>
#include <regex>

std::string SQL_Utils::hashedToBase64(const std::string& input) {
    // Step 1: Compute SHA-256 hash
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), input.size(), hash);

    // Step 2: Base64 encode the hash
    BIO* bio, * b64;
    BUF_MEM* bufferPtr;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);

    // Don’t use newlines in Base64 output
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);

    BIO_write(bio, hash, SHA256_DIGEST_LENGTH);
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bufferPtr);

    std::string base64Hash(bufferPtr->data, bufferPtr->length);
    BIO_free_all(bio);

    for(int i = 0; i<base64Hash.size(); i++) {
        if(base64Hash[i]=='/') base64Hash[i] = '-';
    }
    return base64Hash;
}

std::string SQL_Utils::replacedUnicodeTags(const std::string& input) {
    std::regex pattern(R"(\[U\+0*([0-9A-Fa-f]+)\])");
    std::string output;
    std::sregex_iterator it(input.begin(), input.end(), pattern);
    std::sregex_iterator end;

    size_t lastPos = 0;

    while (it != end) {
        const std::smatch &m = *it;

        // Append text before match
        output.append(input, lastPos, m.position() - lastPos);

        // Convert hex code to an integer
        unsigned int cp = std::stoul(m[1].str(), nullptr, 16);

        // Append UTF-8 char
        output += codepointToUTF8(cp);

        // Move past the match
        lastPos = m.position() + m.length();
        ++it;
    }

    // Append remaining text
    output.append(input, lastPos, std::string::npos);

    return output;
}

// Encode a Unicode code point (U+0000..U+10FFFF) as UTF-8
std::string SQL_Utils::codepointToUTF8(unsigned int cp) {
    std::string out;

    if (cp <= 0x7F) {
        out.push_back(static_cast<char>(cp));
    }
    else if (cp <= 0x7FF) {
        out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
    else if (cp <= 0xFFFF) {
        out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
    else if (cp <= 0x10FFFF) {
        out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
    else {
        throw std::runtime_error("Invalid Unicode code point");
    }

    return out;
}
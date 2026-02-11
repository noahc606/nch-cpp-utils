#pragma once
#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

namespace nch { class StringUtils {
public:

    static void logValidationWarnings(bool show);

    template<typename T> static void cat(std::stringstream& ss, T t) {
        ss << t;
    }
    template<typename T, typename... Args>static void cat(std::stringstream& ss, T first, Args... args) {
        ss << first; cat(ss, args...);
    }
    template<typename T, typename... Args>static std::string cat(T first, Args... args) {
        std::stringstream ss; cat(ss, first, args...); return ss.str();
    }

    /// @brief Similar to Java's String.split(regex), but instead of a regex, 'delim' is a char to split by.
    /// @brief Size-0 strings will not appear in the resulting vec.
    /// @brief Example: split("this,is,a,test", ',') => {"this", "is", "a", "test"}.
    /// @param toSplit The string to be split into an array.
    /// @param delim A char to split the string 'toSplit' by.
    /// @return a vector of strings where each element is a substring within 'toSplit', split by 'delim'.
    static std::vector<std::string> split(const std::string& toSplit, char delim);
    /// @brief Take in a string of the form "[#, #, #, ..., #]" and return a vector of int64s. Returns empty vector if a bad format is given.
    /// @param s The string to parse which is an I64 array.
    /// @return A parsed vector of int64_ts.
    static std::vector<int64_t> parseI64Array(std::string s);
    static std::vector<double> parseDoubleArray(std::string s);
    /// @brief Same as parseI64Array, but uses strings of the form "# # # ... #". Add 0 for every non-integer found.
    /// @param s The string to parse which is a simple I64 array.
    /// @return Return a list of I64s which is represented by 's'.
    static std::vector<int64_t> parseI64ArraySimple(const std::string& s);
    static std::string vecToArrayString(const std::vector<std::string>& v);

    /// @brief Search from either end of 's' until we find '[' from the left and ']' from the right.
    /// @param s The bracketed string to extract the contents of.
    /// @return a string of the form "[...]" or "" if we couldn't find a proper bracketed string.
    static std::string extractBracketedStr(const std::string& s);
    static std::string trimmed(const std::string& s, const std::string& charsToTrim);
    static std::string trimmed(const std::string& s);
    static std::string removedNonASCII(const std::string& s);
    static std::string unicodeEscaped(const std::wstring& ws);
    static std::string shortened(const std::string& s, int maxDisplaySize = 64);

    static std::string stringFromBytestream(const std::vector<unsigned char>& byteStream, bool keepZeros = false);
    static std::vector<unsigned char> bytestreamFromString(const std::string& str);
    static bool aHasPrefixB(const std::string& a, const std::string& b);
    static bool aHasSuffixB(const std::string& a, const std::string& b);
    static bool aContainsB(const std::string& a, const std::string& b);
    static bool aContainsAllMembersOfB(const std::string& a, const std::vector<std::string>& b);
    static std::string replacedAllAWithB(std::string str, const std::string& a, const std::string& b);
    static std::u16string u16ReplacedAllAWithB(std::u16string str, const std::u16string& a, const std::u16string& b);

    static int parseCmdArg(const std::vector<std::string>& args, std::string argLabel, int defaultValue, int errorValue);
    static std::string parseCmdArg(const std::vector<std::string>& args, std::string argLabel, std::string defaultValue, std::string errorValue);
    static bool cmdArgExists(const std::vector<std::string>& args, std::string arg);

    static std::string validatedString(const std::string& s, const std::string& charSet);
    static bool validateString(const std::string& s, const std::string& charSet);
    static bool validateAlphanumeric(const std::string& s);
    static bool validateInjectionless(const std::string& s);
    static bool validateSpaceless(const std::string& s);
    static bool validateCmdArgLabel(const std::string& s);
    static bool validateSafeString(const std::string& s);
    static bool validateIP(const std::string& s);
    static bool validateEnv(const std::string& s);
private:
    static bool loggingValidationWarnings;
}; }
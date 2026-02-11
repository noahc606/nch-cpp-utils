#include "StringUtils.h"
#include "nch/cpp-utils/log.h"
#include <codecvt>
#include <cstring>
#include <iomanip>
#include <locale>
#include <sstream>
using namespace nch;
bool StringUtils::loggingValidationWarnings = true;

void StringUtils::logValidationWarnings(bool show) {
    loggingValidationWarnings = show;
}

std::vector<std::string> StringUtils::split(const std::string& toSplit, char delim)
{
    std::string delims(1, delim);
    std::vector<std::string> res;
    if(toSplit.size()==0) return res;

    //List all locations of instances of the 'delim' string
    std::vector<int> posList;
    for(int i = 0; i<toSplit.size(); i++) {
        if(toSplit[i]==delim) {
            posList.push_back(i);
        }
    }
    posList.push_back(toSplit.size());
    
    //Split string, add to 'res'
    std::string potentialFirst = toSplit.substr(0, posList[0]);
    if(potentialFirst.size()>0) {
        res.push_back(potentialFirst);
    }
    for(int i = 0; i<posList.size()-1; i++) {
        std::string potential = toSplit.substr(posList[i]+1, posList[i+1]-posList[i]-1);
        if(potential.size()>0) {
            res.push_back(potential);
        }
    }

    //Return 'res'
    return res;
}

std::vector<int64_t> StringUtils::parseI64Array(std::string s)
{
    /* 2BReturned */
    std::vector<int64_t> res;

    /* Cleanup */
    //Clear whitespace
    s = replacedAllAWithB(s, " ", "");
    //Check for left and right bracket. If nonexistent, stop.
    if(s[0]!='[' || s[s.size()-1]!=']') {
        return res;
    }
    //Remove brackets
    s = s.substr(1, s.size()-2);

    /* Splitting + Parse #s */
    auto spl = split(s, ',');
    for(int i = 0; i<spl.size(); i++) {
        try {
            res.push_back(std::atoll(spl[i].c_str()));
        } catch(...) {
            res.push_back(0);
            Log::warnv(__PRETTY_FUNCTION__, "inserting 0 into 'res'", "Failed to parse int64_t from split string element \"%s\"", spl[i].c_str());
        }
    }

    /* Return */
    return res;
}
std::vector<double> StringUtils::parseDoubleArray(std::string s)
{
    /* 2B'Ret'urned */
    std::vector<double> ret;

    /* Cleanup */
    //Clear whitespace
    s = replacedAllAWithB(s, " ", "");
    //Check for left and right bracket. If nonexistent, stop.
    if(s[0]!='[' || s[s.size()-1]!=']') {
        return ret;
    }
    //Remove brackets
    s = s.substr(1, s.size()-2);

    /* Splitting + Parse #s */
    auto spl = split(s, ',');
    for(int i = 0; i<spl.size(); i++) {
        try {
            ret.push_back(std::stod(spl[i].c_str()));
        } catch(...) {
            ret.push_back(0);
            Log::warnv(__PRETTY_FUNCTION__, "inserting 0 into 'res'", "Failed to parse int64_t from split string element \"%s\"", spl[i].c_str());
        }
    }

    /* Return */
    return ret;
}
std::vector<int64_t> StringUtils::parseI64ArraySimple(const std::string& s)
{
    /* 2BReturned */
    std::vector<int64_t> res;

    /* Splitting + Parse #s */
    auto spl = split(s, ' ');
    for(int i = 0; i<spl.size(); i++) {
        try {
            res.push_back(std::atoll(spl[i].c_str()));
        } catch(...) {
            res.push_back(0);
        }
    }

    /* Return */
    return res;
}
std::string StringUtils::vecToArrayString(const std::vector<std::string>& v)
{
    std::stringstream ret;
    ret << "[";
    for(int i = 0; i<v.size(); i++) {
        ret << v[i];
        if(i<v.size()-1) ret << ", ";
    }
    ret << "]";
    return ret.str();
}
std::string StringUtils::extractBracketedStr(const std::string& s)
{
    int lBktPos = -1; for(int i = 0; i<s.size(); i++)    if(s[i]=='[') lBktPos = i;
    int rBktPos = -1; for(int i = s.size()-1; i>=0; i--) if(s[i]==']') rBktPos = i;

    if(rBktPos>lBktPos && lBktPos!=-1 && rBktPos!=-1) {
        return s.substr(lBktPos, rBktPos-lBktPos);
    } else {
        return "][";
    }
}

std::string StringUtils::trimmed(const std::string& s, const std::string& charsToTrim)
{
    //Characters to be trimmed at beginning or end
    std::string ctt = charsToTrim;

    //Find first non-whitespace character from beginning
    int start = 0;
    for(int i = start; i<s.size(); i++) {
        if(ctt.find(s[i])==std::string::npos) { start = i; break; }
        //Special case: All characters found to be whitespace -> return "".
        if(i==s.size()-1) {
            return "";
        }
    }
    //Find first non-whitespace character from end
    int end = s.size()-1;
    for(int i = end; i>=0; i--) {
        if(ctt.find(s[i])==std::string::npos) { end = i+1; break; }
    }

    //Return final substring
    return s.substr(start, end-start);
}
std::string StringUtils::trimmed(const std::string& s)
{
    //Trim whitespace characters
    return trimmed(s, " \t\n");
}
std::string StringUtils::removedNonASCII(const std::string& s)
{
    std::stringstream ret;
    for(int i = 0; i<s.size(); i++) {
        if(s[i]>=32 && s[i]<=126)
            ret << s[i];
    }
    return ret.str();
}
std::string StringUtils::unicodeEscaped(const std::wstring& ws)
{
    std::ostringstream ret;
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;

    for(wchar_t wc : ws) {
        //Transform certain wchars into ASCII versions
        if(wc==0x9) { ret << "\t"; continue; } //Horizontal Tabulation -> ascii tab
        if(wc==0xa) { ret << "\n"; continue; } //Line feed -> ascii newline
        //Check if wchar is printable and in BMP or valid UTF-8 range
        if(wc>=0x20 && wc<=0x7E) {
            //ASCII printable characters: convert directly
            ret << static_cast<char>(wc);
        } else {
            ret << "[U+"
                << std::uppercase << std::hex << std::setfill('0')
                << static_cast<int>(wc)
                << "]";
        }
    }

    return ret.str();
}
std::string StringUtils::shortened(const std::string& s, int maxDisplaySize) {
    if(s.size()<=maxDisplaySize) {
        return s;
    }
    int segSize = (maxDisplaySize-3)/2;
    std::string beg = s.substr(0, segSize);
    std::string end = s.substr(s.size()-segSize, segSize);
    return beg+"..."+end;
}

std::string StringUtils::stringFromBytestream(const std::vector<unsigned char>& byteStream, bool keepZeros)
{
    std::stringstream ret("");
    if(keepZeros) {
        for(int i = 0; i<byteStream.size(); i++) {
            ret << (char)byteStream[i];
        }
    } else {
        for(int i = 0; i<byteStream.size(); i++) {
            if(byteStream[i]!='\0')
                ret << (char)byteStream[i];
        }
    }
    return ret.str();
}
std::vector<unsigned char> StringUtils::bytestreamFromString(const std::string& str)
{
    std::vector<unsigned char> ret;
    ret.reserve(str.size());
    for(int i = 0; i<str.size(); i++) {
        ret.push_back((unsigned char)str[i]);
    }
    return ret;
}

/*
    Returns: Whether or not the string 's' has 'prefix's string sequence at the beginning.
*/
bool StringUtils::aHasPrefixB(const std::string& a, const std::string& b)
{
    try        { return (a.substr(0, b.size())==b); }
    catch(...) { return false; }
    
}

bool StringUtils::aHasSuffixB(const std::string& a, const std::string& b)
{
    try        { return (a.substr(a.size()-b.size())==b); }
    catch(...) { return false; }
}

bool StringUtils::aContainsB(const std::string& a, const std::string& b) {
    return (a.find(b)!=std::string::npos);    
}

bool StringUtils::aContainsAllMembersOfB(const std::string& a, const std::vector<std::string>& b) {
    for(int i = 0; i<b.size(); i++) {
        if(!aContainsB(a, b[i])) return false;
    }
    return true;
}
std::string StringUtils::replacedAllAWithB(std::string str, const std::string& a, const std::string& b)
{
    size_t pos = str.find(a);
    while(pos!=std::string::npos) {
        str.replace(pos, a.size(), b);
        pos = str.find(a, pos + b.size());
    }
    return str;
}
std::u16string StringUtils::u16ReplacedAllAWithB(std::u16string str, const std::u16string& a, const std::u16string& b)
{
    size_t pos = str.find(a);
    while(pos!=std::u16string::npos) {
        str.replace(pos, a.size(), b);
        pos = str.find(a, pos + b.size());
    }
    return str;
}

int StringUtils::parseCmdArg(const std::vector<std::string>& args, std::string argLabel, int defaultValue, int errorValue)
{
    if(!validateCmdArgLabel(argLabel)) { return errorValue; }

    int res = defaultValue;
    for(int i = 0; i<args.size(); i++) {
        if(StringUtils::aHasPrefixB(args[i], argLabel+"=")) {
            try        { res = std::stoi(args[i].substr(argLabel.size()+1)); }
            catch(...) { res = errorValue; }
            break;
        }
        if(args[i]==argLabel && i<args.size()-1) {
            try        { res = std::stoi(args[i+1]); }
            catch(...) { res = errorValue; }
            break;
        }
    }

    return res;
}

std::string StringUtils::parseCmdArg(const std::vector<std::string>& args, std::string argLabel, std::string defaultValue, std::string errorValue)
{
    if(!validateCmdArgLabel(argLabel)) return errorValue;

    std::string res = defaultValue;
    for(int i = 0; i<args.size(); i++) {
        if(StringUtils::aHasPrefixB(args[i], argLabel+"=")) {
            try        { res = args[i].substr(argLabel.size()+1); }
            catch(...) { res = errorValue; }
            if(res=="") res = errorValue;
            break;
        }
        if(args[i]==argLabel && i<args.size()-1) {
            try        { res = args[i+1]; }
            catch(...) { res = errorValue; }
            if(res=="") res = errorValue;
            break;
        }
    }

    return res;
}

bool StringUtils::cmdArgExists(const std::vector<std::string>& args, std::string arg)
{
    for(int i = 0; i<args.size(); i++) {
        if(args[i]==arg) return true;
    }
    return false;
}
std::string StringUtils::validatedString(const std::string& s, const std::string& charSet)
{
    std::stringstream ret;
    for(int i = 0; i<s.size(); i++)
        if(charSet.find(s[i])!=std::string::npos) {
            ret << s[i];
        }
    return ret.str();
}

bool StringUtils::validateString(const std::string& s, const std::string& charSet)
{
    for(int i = 0; i<s.size(); i++)
        if(charSet.find(s[i])==std::string::npos) {
            if(loggingValidationWarnings)
                Log::warnv(__PRETTY_FUNCTION__, "returning false", "String validation failed (\"%s\" must only contain characters from \"%s\")", s.c_str(), charSet.c_str());
            return false;
        }
    return true;
}

bool StringUtils::validateAlphanumeric(const std::string& s) {
    return validateString(s, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
}
bool StringUtils::validateInjectionless(const std::string& s) {
    return validateString(s, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_");
}
bool StringUtils::validateCmdArgLabel(const std::string& s) {
    return validateString(s, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-");
}
bool StringUtils::validateSpaceless(const std::string& s) {
    return validateString(s, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:/.?!@#$%^&*=#_-~+");
}
bool StringUtils::validateSafeString(const std::string& s) {
    return validateString(s, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_ ()[]{}'");
}

bool StringUtils::validateIP(const std::string& s) {
    return validateString(s, "0123456789.");
}

bool StringUtils::validateEnv(const std::string& s) {
    if(s.size()==0 || s.size()>1024) return false;
    if(!validateString(s, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_")) return false;
    if(s.at(0)>='0' && s.at(0)<='9') return false;
    return true;
}
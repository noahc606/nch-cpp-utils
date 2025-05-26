#include "StringUtils.h"
#include "log.h"
#include <sstream>


using namespace nch;

/*
    Similar to Java's String.split(regex), but instead of a regex, 'delim' is a char to split by.
    Size-0 strings will not appear in the resulting vec.
    
    - Returns: a vector of strings where each element is a substring within 'toSplit', split by 'delim'.
    - Example: split("this,is,a,test", ',') => {"this", "is", "a", "test"}.
*/
std::vector<std::string> StringUtils::split(std::string toSplit, char delim)
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

/*
    Take in a string of the form "[#, #, #, ..., #]" and return a vector of int64s.

    - Returns: A parsed vector of int64_ts.
*/
std::vector<int64_t> StringUtils::parseI64Array(std::string s)
{
    /* 2BReturned */
    std::vector<int64_t> res;

    /* Cleanup */
    //Clear whitespace
    std::stringstream ss1;
    for(int i = 0; i<s.size(); i++) {
        if(s[i]!=' ') {
            ss1 << s[i];
        }
    }
    std::string s1 = ss1.str();
    //Check for left and right bracket. If nonexistent, stop.
    if(s1[0]!='[' || s1[s1.size()-1]!=']') {
        Log::warnv(__PRETTY_FUNCTION__, "returning empty vector", "Failed to parse array from string \"%s\"", s1.c_str());
        return res;
    }
    //Remove brackets
    s1 = s1.substr(1, s1.size()-2);

    /* Splitting + Parse #s */
    auto spl = split(s1, ',');
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

/*
    Same as parseI64Array, but uses strings of the form "# # # ... #". Add 0 for every non-integer found.
*/
std::vector<int64_t> StringUtils::parseI64ArraySimple(std::string s)
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

/*
    Search from either end of 's' until we find '[' from the left and ']' from the right.
    Returns: a string of the form "[...]" or "" if we couldn't find a proper bracketed string.
*/
std::string StringUtils::extractBracketedStr(std::string s)
{
    int lBktPos = -1; for(int i = 0; i<s.size(); i++)    if(s[i]=='[') lBktPos = i;
    int rBktPos = -1; for(int i = s.size()-1; i>=0; i--) if(s[i]==']') rBktPos = i;

    if(rBktPos>lBktPos && lBktPos!=-1 && rBktPos!=-1) {
        return s.substr(lBktPos, rBktPos-lBktPos);
    } else {
        return "][";
    }
}

std::string StringUtils::trimmed(std::string s)
{
    //Whitespace characters to be trimmed at beginning or end
    std::string ws = " \t\n";

    //Find first non-whitespace character from beginning
    int start = 0;
    for(int i = start; i<s.size(); i++) {
        if(ws.find(s[i])==std::string::npos) { start = i; break; }
        //Special case: All characters found to be whitespace -> return "".
        if(i==s.size()-1) {
            return "";
        }
    }
    //Find first non-whitespace character from end
    int end = s.size()-1;
    for(int i = end; i>=0; i--) {
        if(ws.find(s[i])==std::string::npos) { end = i+1; break; }
    }

    //Return final substring
    return s.substr(start, end-start);
}

std::string StringUtils::fromByteStream(std::vector<char>& byteStream)
{
    std::stringstream res("");
    for(int i = 0; i<byteStream.size(); i++) {
        if(byteStream[i]=='\0') return res.str();
        res << byteStream[i];
    }
    return res.str();
}

/*
    Returns: Whether or not the string 's' has 'prefix's string sequence at the beginning.
*/
bool StringUtils::aHasPrefixB(const std::string& a, const std::string& b)
{
    return (a.substr(0, b.size())==b);
}

bool StringUtils::aHasSuffixB(const std::string& a, const std::string& b)
{
    return(a.substr(a.size()-b.size())==b);
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


bool StringUtils::validateString(std::string s, std::string charSet)
{
    for(int i = 0; i<s.size(); i++)
        if(charSet.find(s[i])==std::string::npos) {
            Log::warnv(__PRETTY_FUNCTION__, "returning false", "String validation failed");
            return false;
        }
    return true;
}

bool StringUtils::validateAlphanumeric(std::string s) {
    return validateString(s, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
}
bool StringUtils::validateInjectionless(std::string s) {
    return validateString(s, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_");
}
bool StringUtils::validateSpaceless(std::string s) {
    return validateString(s, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:/.?=&#_-~%+");
}
bool StringUtils::validateSafeString(std::string s) {
    return validateString(s, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_ ()[]{}'");
}
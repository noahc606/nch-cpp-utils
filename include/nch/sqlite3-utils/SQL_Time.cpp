#include "SQL_Time.h"
#include <iomanip>
#include <map>
#include <regex>
#include <stdexcept>
#include <time.h>

SQL_Time::SQL_Time() {
    timestamp = "1900-01-01 00:00:00";
    std::time_t ntime = 0;
    timeTM = *gmtime(&ntime);
}
SQL_Time SQL_Time::fromNow(bool utc) {
    SQL_Time ret;
    time_t now = std::time(NULL);
    if(utc) { ret.timeTM = *std::gmtime(&now); }
    else    { ret.timeTM = *std::localtime(&now); }
    ret.timestamp = toSQLDateTime(ret.timeTM);
    return ret;
}
SQL_Time SQL_Time::fromInterpreted(std::time_t time, bool utc) {
    SQL_Time ret;
    if(utc) { ret.timeTM = *std::gmtime(&time); }
    else    { ret.timeTM = *std::localtime(&time); }
    ret.timestamp = toSQLDateTime(ret.timeTM);
    return ret;
}
SQL_Time::SQL_Time(std::tm tm) {
    timeTM = tm;
    timestamp = toSQLDateTime(timeTM);
}
SQL_Time::SQL_Time(const std::string& timestampRaw) {
    timeTM = toTimeTM(timestampRaw);
    timestamp = toSQLDateTime(timeTM);
}
SQL_Time::~SQL_Time(){}

std::tm SQL_Time::getStartOfDay(std::tm tm) {
    tm.tm_hour = 0;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    mktime(&tm);
    return tm;
}
std::tm SQL_Time::getStartOfYear(std::tm tm) {
    tm.tm_mon = 0;
    tm.tm_mday = 1;
    tm.tm_hour = 0;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    mktime(&tm);
    return tm;
}
std::tm SQL_Time::addDays(std::tm tm, int days) {
    time_t rawTime = mktime(&tm);
    rawTime += days * 86400;
    struct tm *newTime = localtime(&rawTime);
    tm = *newTime;
    return tm;
}
std::tm SQL_Time::addMonths(std::tm tm, int months) {
    tm.tm_mon += months;
    if(tm.tm_mon>11) {
        int yearAdjustment = tm.tm_mon/12;
        tm.tm_mon = tm.tm_mon % 12;
        tm.tm_year += yearAdjustment;
    } 
    else if(tm.tm_mon<0) {
        int yearAdjustment = (tm.tm_mon/12)-1;
        tm.tm_mon = (tm.tm_mon+12)%12;
        tm.tm_year += yearAdjustment;
    }
    mktime(&tm);
    return tm;
}
std::tm SQL_Time::addYears(std::tm tm, int years) {
    tm.tm_year += years;
    if(tm.tm_mon==1 && tm.tm_mday==29) {
        if((tm.tm_year+1900)%4!=0 || ((tm.tm_year+1900)%100==0 && (tm.tm_year+1900)%400!=0)) {
            tm.tm_mday = 28;
        }
    }
    
    mktime(&tm);
    return tm;
}

std::tm SQL_Time::toTimeTM(const std::string& timestampRaw) {
    std::string input = timestampRaw;
    std::transform(input.begin(), input.end(), input.begin(),
                   [](unsigned char c){ return std::tolower(c); });

    // --- Find Year (4 digits)
    std::smatch yearMatch;
    int year = -987354311;
    if(std::regex_search(input, yearMatch, std::regex(R"(\b(\d{4})\b)"))) {
        try {
            year = std::stoi(yearMatch.str(1));
        } catch(...) {}
    }
    if(year==-987354311) {
        std::time_t now = std::time(nullptr);
        std::tm* localTime = std::localtime(&now);
        year = localTime->tm_year+1900;
    }

    // --- Find Month (jan, feb, ..., dec)
    static const std::map<std::string, int> monthMap = {
        {"jan",1},{"feb",2},{"mar",3},{"apr",4},{"may",5},{"jun",6},
        {"jul",7},{"aug",8},{"sep",9},{"oct",10},{"nov",11},{"dec",12}
    };
    int month = 0;
    size_t monthPos = std::string::npos;
    std::string foundMonth;

    for(auto const& elem : monthMap) {
        size_t pos = input.find(elem.first);
        if(pos != std::string::npos) {
            month = elem.second;
            monthPos = pos;
            foundMonth = elem.first;
            break;
        }
    }

    // --- Find Day (1–2 digit number either before OR after month)
    int day = -1;
    std::smatch matchBefore, matchAfter;
    // Word boundary ensures standalone 1-2 digits (not part of a year)
    std::regex beforeMonth(R"(\b(\d{1,2})\b[^a-z]{0,3})");
    std::regex afterMonth(R"([^a-z]{0,3}\b(\d{1,2})\b)");

    if (std::regex_search(input, matchBefore, beforeMonth)) {
        day = std::stoi(matchBefore.str(1));
    } else if (std::regex_search(input, matchAfter, afterMonth)) {
        day = std::stoi(matchAfter.str(1));
    }

    // --- Find Time
    std::regex timeRegex(R"((\d{1,2}):(\d{1,2})(?::(\d{1,2}))?)");
    std::smatch timeMatch;
    int hour = -1, minute = -1, second = -1;
    if(!std::regex_search(input, timeMatch, timeRegex)) {
        //No time found, keep all as -1.
    } else {
        hour = std::stoi(timeMatch.str(1));
        minute = std::stoi(timeMatch.str(2));
        second = (timeMatch.size() > 3 && timeMatch.str(3).size()) ? std::stoi(timeMatch.str(3)) : 0;
    }

    // --- Handle AM/PM
    bool hasAM = input.find("am") != std::string::npos;
    bool hasPM = input.find("pm") != std::string::npos;
    if (hasAM || hasPM) {
        if (hour == 12) hour = 0; // midnight
        if (hasPM) hour += 12;    // afternoon case
    }

    //Convert to time_
    //Time only = current month, day, and year
    if((month<=0 || day<=0) && hour>=0 && minute>=0 && second>=0) {
        std::time_t now = std::time(nullptr);
        std::tm* localTime = std::localtime(&now);
        year = localTime->tm_year+1900;
        month = localTime->tm_mon+1;
        day = localTime->tm_mday;
    }
    //Month only = first day of month 00:00:00
    if(month<=0) {
        month = 1;
        day = 0;
    }
    //No day = day 1 00:00:00
    if(day<=0) {
        day = 1;
        hour = 0; minute = 0; second = 0;
    }
    //If any of hour/minute/day is broken: Set to 0, 0, 0
    if(hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) {
        hour = 0; minute = 0; second = 0;
    }

    std::tm tm = {};
    tm.tm_year = year-1900;
    tm.tm_mon = month-1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = second;
    tm.tm_isdst = -1;
    return tm;
}
std::string SQL_Time::toSQLDateTime(std::tm tm) {
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::time_t SQL_Time::interpret(bool utc) {
    if(!utc) {
        return std::mktime(&timeTM);
    }

    #if defined(_WIN32)
        return _mkgmtime(&timeTM);
    #else
        return timegm(&timeTM);
    #endif
}
#pragma once
#include <string>
#include <ctime>
class SQL_Time {
public:
    SQL_Time();
    SQL_Time(std::tm tm);
    SQL_Time(const std::string& timestamp);
    ~SQL_Time();
    static SQL_Time fromNow(bool utc);
    static SQL_Time fromInterpreted(std::time_t time, bool utc);

    static std::tm getStartOfDay(std::tm tm);
    static std::tm getStartOfYear(std::tm tm);
    static std::tm addDays(std::tm tm, int days);
    static std::tm addMonths(std::tm tm, int months);
    static std::tm addYears(std::tm tm, int years);

    static std::tm toTimeTM(const std::string& timestampRaw);
    static std::string toSQLDateTime(std::tm tm);
    std::time_t interpret(bool utc);

    std::string timestamp = "";
    std::tm timeTM;
private:
};
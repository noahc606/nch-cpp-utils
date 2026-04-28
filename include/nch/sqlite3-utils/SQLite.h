#pragma once
#include <cxxabi.h>
#include <iostream>
#include <ostream>
#include <nch/cpp-utils/log.h>
#include <nch/cpp-utils/string-utils.h>
#include <nlohmann/json.hpp>
#include <sqlite3.h>
#include <string>
#include <tuple>
#include "SQL_Time.h"

class SQLite {
public:
    static sqlite3* open(const std::string& dbPath);
    static void close(sqlite3* db);
    static std::string getName(sqlite3* db);
    static void exec(sqlite3 *db, const std::string& query);
    static std::vector<nlohmann::json> execPreppedStatement(sqlite3* db, const std::string& query);
    static int getNumRows(sqlite3* db, const std::string& tableName);

    template<typename... Args>
    static bool insertData(sqlite3* db, const std::string& tableName, const Args&... args) {
        if(!db) {
            nch::Log::error(__PRETTY_FUNCTION__, "db==nullptr");
            return false;
        }
        if(!nch::StringUtils::validateInjectionless(tableName)) {
        nch::Log::error(__PRETTY_FUNCTION__, "'tableName' not injectionless.");
            return false;
        }
        constexpr size_t numArgs = sizeof...(args);
        if(numArgs==0) {
            nch::Log::error(__PRETTY_FUNCTION__, "No data provided to insert.");
            return false;
        }

        //Build SQL query: INSERT INTO tableName VALUES (?, ?, ?, ...)
        std::ostringstream oss; {
            oss << "INSERT INTO " << tableName << " VALUES (";
            for (size_t i = 0; i < numArgs; ++i) {
                oss << "?";
                if (i < numArgs - 1)
                    oss << ", ";
            }
            oss << ");";
        }
        std::string sql = oss.str();

        //Prepare SQL statement
        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
        if(rc!=SQLITE_OK) {
            nch::Log::errorv(__PRETTY_FUNCTION__, "SQL prepare error", sqlite3_errmsg(db));
            return false;
        }
        //Bind parameters to statement
        rc = bindParams(stmt, 1, args...);
        if(rc!=SQLITE_OK) {
            nch::Log::errorv(__PRETTY_FUNCTION__, "Bind error", sqlite3_errmsg(db));
            sqlite3_finalize(stmt);
            return false;
        }

        //Execute statement
        rc = sqlite3_step(stmt);
        if(rc!=SQLITE_DONE) {
            sqlite3_finalize(stmt);
            return false;
        }

        sqlite3_finalize(stmt);
        return true;
    }
private:
    static inline int bindParams(sqlite3_stmt* stmt, int) {
        return SQLITE_OK;
    }

    template<typename T, typename... Rest>
    static int bindParams(sqlite3_stmt* stmt, int index, const T& value, const Rest&... rest) {
        std::string vtype; {
            int status;
            char* demangled = abi::__cxa_demangle(typeid(value).name(), 0, 0, &status);
            vtype = (status == 0 ? demangled : typeid(value).name());
            std::free(demangled);
        }

        int rc = SQLITE_ERROR;
        if(typeid(value)==typeid(int)) {
            rc = sqlite3_bind_int(stmt, index, *(int*)&value);
        } else
        if(typeid(value)==typeid(std::string)) {
            rc = sqlite3_bind_text(stmt, index, ((std::string*)&value)->c_str(), -1, SQLITE_TRANSIENT);
        } else
        if(typeid(value)==typeid(SQL_Time)) {
            rc = sqlite3_bind_text(stmt, index, ((SQL_Time*)&value)->timestamp.c_str(), -1, SQLITE_TRANSIENT);
        } else
        if(typeid(value)==typeid(char[5])) {
            if((std::string)((char*)&value)=="null") {
                rc = sqlite3_bind_null(stmt, index);
            } else {
                vtype = "char*!='null'";
            }
        }

        if(rc==SQLITE_ERROR) {
            nch::Log::error(__PRETTY_FUNCTION__, "Unsupported type for parameter %d w/ type \"%s\"", index, vtype.c_str());
            return SQLITE_MISMATCH;
        }
        if(rc!=SQLITE_OK)
            return rc;

        // Recurse for the next parameter
        return bindParams(stmt, index + 1, rest...);
    }
};
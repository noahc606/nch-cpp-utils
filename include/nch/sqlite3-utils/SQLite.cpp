#include "SQLite.h"
#include <nch/cpp-utils/log.h>
#include <nch/cpp-utils/string-utils.h>
#include <vector>
using namespace nch;

sqlite3* SQLite::open(const std::string& dbPath) {
    sqlite3* ret;
    if(sqlite3_open(dbPath.c_str(), &ret)) {
        Log::errorv(__PRETTY_FUNCTION__, (char*)sqlite3_errmsg16(ret), "Failed to open open database \"%s\"", dbPath.c_str());
        throw std::invalid_argument("");
    }
    return ret;
}
void SQLite::close(sqlite3* db) {
    sqlite3_close(db);
}

std::string SQLite::getName(sqlite3* db)
{
    const char* ret = sqlite3_db_filename(db, "main");
    if(ret==nullptr) {
        Log::error(__PRETTY_FUNCTION__, "Error retrieving database filename");
    }
    return ret;
}
void SQLite::exec(sqlite3* db, const std::string& query)
{
    char *errMsg = 0;
    int ret = sqlite3_exec(db, query.c_str(), 0, 0, &errMsg);
    if(ret!=SQLITE_OK) {
        Log::errorv(__PRETTY_FUNCTION__, errMsg, "SQL Execution Error for database \"%s\"", getName(db).c_str());
        sqlite3_free(errMsg);
    }
}
std::vector<nlohmann::json> SQLite::execPreppedStatement(sqlite3* db, const std::string& query) {
    std::vector<nlohmann::json> ret;
    if(!db) {
        Log::error(__PRETTY_FUNCTION__, "db==nullptr");
        return ret;
    }

    sqlite3_stmt* stmt;
    std::string sql = query;
    if(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        Log::errorv(__PRETTY_FUNCTION__, sqlite3_errmsg(db), "Failed to prepare statement for database \"%s\"", getName(db).c_str());
        return ret;
    }

    while(sqlite3_step(stmt)==SQLITE_ROW) {
        nlohmann::json rowJson;
        int colCount = sqlite3_column_count(stmt);

        for (int i = 0; i < colCount; ++i) {
            const char* colName = sqlite3_column_name(stmt, i);
            int colType = sqlite3_column_type(stmt, i);

            switch (colType) {
                case SQLITE_INTEGER:
                    rowJson[colName] = sqlite3_column_int64(stmt, i);
                    break;
                case SQLITE_FLOAT:
                    rowJson[colName] = sqlite3_column_double(stmt, i);
                    break;
                case SQLITE_TEXT:
                    rowJson[colName] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
                    break;
                case SQLITE_NULL:
                    rowJson[colName] = nullptr;
                    break;
                case SQLITE_BLOB:
                    rowJson[colName] = nullptr; // or handle BLOB specially
                    break;
                default:
                    rowJson[colName] = nullptr;
            }
        }

        ret.push_back(rowJson);
    }

    sqlite3_finalize(stmt);

    return ret;
}

int SQLite::getNumRows(sqlite3* db, const std::string& tableName)
{
    int ret = 0;
    if(!db) {
        Log::error(__PRETTY_FUNCTION__, "db==nullptr");
        return -1;
    }
    if(!StringUtils::validateInjectionless(tableName)) {
        Log::error(__PRETTY_FUNCTION__, "'tableName' not injectionless.");
        return -1;
    }

    sqlite3_stmt* stmt;
    std::string sql = StringUtils::cat("SELECT COUNT(*) FROM ", tableName);
    if(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        Log::errorv(__PRETTY_FUNCTION__, sqlite3_errmsg(db), "Failed to prepare statement for database \"%s\"", getName(db).c_str());
        return ret;
    }

    while(sqlite3_step(stmt)==SQLITE_ROW) {
        ret++;
    }

    sqlite3_finalize(stmt);
    return ret;
}
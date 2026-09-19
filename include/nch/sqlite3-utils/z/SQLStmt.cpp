#include "SQLStmt.h"

using namespace nch;

SQLStmt::SQLStmt(sqlite3* db, const std::string& sql)
{
    if(db==nullptr) return;
    if(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr)!=SQLITE_OK) {
        //prepare_v2 may leave a statement behind even on failure.
        sqlite3_finalize(stmt);
        stmt = nullptr;
    }
}
SQLStmt::~SQLStmt()
{
    sqlite3_finalize(stmt);
}

bool SQLStmt::exists() const { return stmt!=nullptr; }

SQLStmt& SQLStmt::bindBlob(const void* data, int numBytes)
{
    if(stmt!=nullptr) sqlite3_bind_blob(stmt, nextParam++, data, numBytes, SQLITE_TRANSIENT);
    return *this;
}

bool SQLStmt::step()
{
    if(stmt==nullptr) return false;
    return sqlite3_step(stmt)==SQLITE_ROW;
}
bool SQLStmt::run()
{
    if(stmt==nullptr) return false;
    bool done = sqlite3_step(stmt)==SQLITE_DONE;
    sqlite3_reset(stmt);
    nextParam = 1;
    return done;
}

int SQLStmt::getInt(int col) const
{
    if(stmt==nullptr) return 0;
    return sqlite3_column_int(stmt, col);
}
int64_t SQLStmt::getInt64(int col) const
{
    if(stmt==nullptr) return 0;
    return sqlite3_column_int64(stmt, col);
}
std::string SQLStmt::getText(int col) const
{
    if(stmt==nullptr) return "";
    const unsigned char* text = sqlite3_column_text(stmt, col);
    if(text==nullptr) return "";
    return (const char*)text;
}
const uint8_t* SQLStmt::getBlob(int col, int* numBytesOut) const
{
    if(stmt==nullptr) {
        if(numBytesOut!=nullptr) *numBytesOut = 0;
        return nullptr;
    }
    const uint8_t* blob = (const uint8_t*)sqlite3_column_blob(stmt, col);
    if(numBytesOut!=nullptr) *numBytesOut = sqlite3_column_bytes(stmt, col);
    return blob;
}

void SQLStmt::bindNext(){}

void SQLStmt::bindOne(const std::string& value)
{
    if(stmt!=nullptr) sqlite3_bind_text(stmt, nextParam++, value.c_str(), -1, SQLITE_TRANSIENT);
}
void SQLStmt::bindOne(const char* value)
{
    if(stmt!=nullptr) sqlite3_bind_text(stmt, nextParam++, value, -1, SQLITE_TRANSIENT);
}
void SQLStmt::bindInt64(int64_t value)
{
    if(stmt!=nullptr) sqlite3_bind_int64(stmt, nextParam++, value);
}

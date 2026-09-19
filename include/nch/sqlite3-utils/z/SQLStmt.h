#pragma once
#include <cstdint>
#include <sqlite3.h>
#include <string>
#include <type_traits>

namespace nch {

/**
 * @brief An RAII prepared statement: one SQL string, the parameters bound into it, and its result rows.
 *
 * Finalizes in the destructor, so no early return can leak a statement, and folds up the
 * prepare/bind/step/finalize dance that a single row write otherwise costs five lines of.
 *
 * Null-safe the way XMLElem is: a statement that didn't prepare - the usual cause being a table an
 * older DB file predates - reads as one with no rows rather than as an error, so a caller that
 * tolerates that case needs no check at all. Nothing here logs; a prepare failure is a normal,
 * expected outcome for a schema that grew over time.
 */
class SQLStmt {
public:
    SQLStmt(sqlite3* db, const std::string& sql);
    ~SQLStmt();
    SQLStmt(const SQLStmt&) = delete;
    SQLStmt& operator=(const SQLStmt&) = delete;

    //False if the SQL didn't prepare. Every other method is then a no-op yielding the empty value.
    bool exists() const;

    /**
     * @brief Bind the next parameters in '?' order. Integers (and bools) go in as INTEGER, strings as
     *        TEXT; SQLite copies the text, so binding a temporary is safe.
     */
    template<typename... Args> SQLStmt& bind(const Args&... args)
    {
        bindNext(args...);
        return *this;
    }
    SQLStmt& bindBlob(const void* data, int numBytes);

    //Advance to the next result row; false once they run out (or if the statement didn't prepare).
    bool step();
    /**
     * @brief Run a statement that returns no rows, then re-arm it so the next bind() starts over at the
     *        first parameter - which is what lets one prepared INSERT serve a whole loop of rows.
     * @return Whether it ran to completion.
     */
    bool run();

    /**
     * @brief Prepare, bind and run a no-rows statement in one call.
     * @return Whether it ran to completion.
     */
    template<typename... Args> static bool run(sqlite3* db, const std::string& sql, const Args&... args)
    {
        SQLStmt stmt(db, sql);
        return stmt.bind(args...).run();
    }

    /* Column reads, 0-indexed, valid on the row step() just landed on. An absent or NULL column reads
       as the empty value - notably getText(), which yields "" where the raw API yields a null char*. */
    int getInt(int col) const;
    int64_t getInt64(int col) const;
    std::string getText(int col) const;
    //The bytes stay owned by the statement and are invalidated by the next step() or by destruction.
    const uint8_t* getBlob(int col, int* numBytesOut = nullptr) const;
private:
    void bindNext();
    template<typename T, typename... Rest> void bindNext(const T& value, const Rest&... rest)
    {
        bindOne(value);
        bindNext(rest...);
    }
    //Every integer width goes in as an int64: SQLite stores one INTEGER type regardless.
    template<typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
    void bindOne(const T& value)
    {
        bindInt64((int64_t)value);
    }
    void bindOne(const std::string& value);
    void bindOne(const char* value);
    void bindInt64(int64_t value);

    sqlite3_stmt* stmt = nullptr;
    int nextParam = 1;
};

}

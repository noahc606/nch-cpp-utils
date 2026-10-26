#pragma once
#include <sqlite3.h>
#include <string>

namespace nch {

/**
 * @brief An open connection that closes itself, for the usual open-work-close function body.
 *
 * Declare the connection first and its SQLStmts after it, and destruction order does the rest: every
 * statement finalizes before the close. That ordering is not optional - sqlite3_close refuses to
 * close a connection that still holds a prepared statement - and getting it from scope rather than
 * from hand-placed cleanup is the whole point of this type.
 *
 * Converts to sqlite3*, so it passes straight into SQLStmt and the plain sqlite3 API.
 */
class SQLDB {
public:
    enum Access {
        READ_WRITE, //SQLite::open: created if missing, WAL + synchronous=NORMAL applied
        READ_ONLY,  //SQLite::openReadOnly: an existing file, immutable, never written (see there)
    };

    //Throws if the file can't be opened, matching SQLite::open/openReadOnly (whose behavior this takes on).
    SQLDB(const std::string& dbPath, Access access = READ_WRITE);
    ~SQLDB();
    SQLDB(const SQLDB&) = delete;
    SQLDB& operator=(const SQLDB&) = delete;

    sqlite3* get() const;
    operator sqlite3*() const;
private:
    sqlite3* db = nullptr;
};

}

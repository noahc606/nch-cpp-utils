#include "SQLDB.h"
#include "nch/sqlite3-utils/SQLite.h"

using namespace nch;

SQLDB::SQLDB(const std::string& dbPath)
: db(SQLite::open(dbPath)) {}
SQLDB::~SQLDB()
{
    SQLite::close(db);
}

sqlite3* SQLDB::get() const { return db; }
SQLDB::operator sqlite3*() const { return db; }

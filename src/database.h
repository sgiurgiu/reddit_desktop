#ifndef DATABASE_H
#define DATABASE_H

#include <stdexcept>
#include <memory>
#include <optional>

#include "sqlite/sqlite3.h"
#include "entities.h"

class database_exception : public std::runtime_error
{
   using std::runtime_error::runtime_error;
};

struct connection_deleter
{
    void operator()(sqlite3* db)
    {
        if(db)
        {
            sqlite3_close(db);
        }
    }
};

class Database
{
public:
    Database();
    std::optional<user> getRegisteredUser() const;
private:
    std::unique_ptr<sqlite3,connection_deleter> db;
};

#endif // DATABASE_H

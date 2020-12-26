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
private:
    Database();
public:
    static Database* getInstance();
    std::optional<user> getRegisteredUser() const;
    void setRegisteredUser(const user& registeredUser);
    void getMainWindowDimensions(int *x, int *y, int *width,int *height);
    void setMainWindowDimensions(int x, int y, int width,int height);
    void setMediaAudioVolume(int volume);
    int getMediaAudioVolume();
    void setBlurNSFWPictures(bool flag);
    bool getBlurNSFWPictures();
private:
    std::unique_ptr<sqlite3,connection_deleter> db;
    static std::unique_ptr<Database> instance;
};

#endif // DATABASE_H

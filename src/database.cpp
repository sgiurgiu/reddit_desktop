#include "database.h"
#include <SDL.h>

#define DB_ERR_CHECK(msg) \
    if (rc != SQLITE_OK && rc != SQLITE_DONE) \
    {\
        std::string err = sqlite3_errmsg(db.get());\
        int code = sqlite3_errcode(db.get()); \
        throw database_exception(std::string(msg)+":"+err+"("+std::to_string(code)+")");\
    }

namespace
{
    struct statement_finalizer
    {
        void operator()(sqlite3_stmt* stm)
        {
            if(stm)
            {
                sqlite3_finalize(stm);
            }
        }
    };
}

std::unique_ptr<Database> Database::instance(nullptr);

Database::Database():db(nullptr,connection_deleter())
{
    sqlite3* db_ptr;
    int rc = sqlite3_open("rd.db",&db_ptr);
    db.reset(db_ptr);
    DB_ERR_CHECK("Cannot open database");
    rc = sqlite3_exec(db.get(),"CREATE TABLE IF NOT EXISTS USER(USERNAME TEXT, PASSWORD TEXT, CLIENT_ID TEXT, SECRET TEXT, WEBSITE TEXT, APP_NAME TEXT)",nullptr,nullptr,nullptr);
    DB_ERR_CHECK("Cannot create table USER");
    rc = sqlite3_exec(db.get(),"CREATE TABLE IF NOT EXISTS PROPERTIES(NAME TEXT, PROP_VAL TEXT)",nullptr,nullptr,nullptr);
    //X INT, Y INT, WIDTH INT, HEIGHT INT
    DB_ERR_CHECK("Cannot create table WINDOW_DIMS");
}
Database* Database::getInstance()
{
    if(!instance)
    {
        instance = std::unique_ptr<Database>(new Database());
    }
    return instance.get();
}

int Database::getAutoRefreshTimeout() const
{
    return getIntProperty("SUBREDDIT_AUTO_REFRESH",60);
}
void Database::setAutoRefreshTimeout(int value)
{
    setIntProperty(value,"SUBREDDIT_AUTO_REFRESH");
}
void Database::setBoolProperty(bool flag, const std::string& propName)
{
    setIntProperty(flag ? 1 : 0,propName);
}
bool Database::getBoolProperty(const std::string& propName, bool defaultValue) const
{
    return getIntProperty(propName, defaultValue ? 1 : 0) != 0;
}
void Database::setIntProperty(int value, const std::string& propName)
{
    std::string sql("DELETE FROM PROPERTIES WHERE NAME='");
    sql+=propName;
    sql+="'";
    sqlite3_exec(db.get(),sql.c_str(),nullptr,nullptr,nullptr);
    std::unique_ptr<sqlite3_stmt,statement_finalizer> stmt;
    sqlite3_stmt *stmt_ptr;
    int rc = sqlite3_prepare_v2(db.get(),"INSERT INTO PROPERTIES(NAME,PROP_VAL) VALUES(?,?)",-1,&stmt_ptr, nullptr);
    stmt.reset(stmt_ptr);
    DB_ERR_CHECK("Cannot insert "+propName);
    rc = sqlite3_bind_text(stmt.get(),1,propName.c_str(),-1,nullptr);
    DB_ERR_CHECK("Cannot bind values to "+propName)
    rc = sqlite3_bind_int(stmt.get(),2,value);
    DB_ERR_CHECK("Cannot bind values to "+propName);
    rc = sqlite3_step(stmt.get());
    DB_ERR_CHECK("Cannot insert "+propName);
}
int Database::getIntProperty(const std::string& propName, int defaultValue) const
{
    int value = defaultValue;
    std::unique_ptr<sqlite3_stmt,statement_finalizer> stmt;
    sqlite3_stmt *stmt_ptr;
    std::string sql="SELECT NAME,PROP_VAL FROM PROPERTIES WHERE NAME='";
    sql+=propName;
    sql+="'";
    int rc = sqlite3_prepare_v2(db.get(),sql.c_str(),-1,&stmt_ptr, nullptr);
    stmt.reset(stmt_ptr);
    DB_ERR_CHECK("Cannot find "+propName);
    if(sqlite3_step(stmt.get()) == SQLITE_ROW)
    {
        auto name = sqlite3_column_text(stmt.get(),0);
        auto val = sqlite3_column_int(stmt.get(),1);
        if(propName == std::string(reinterpret_cast<const char*>(name)))
        {
             value = val;
        }
    }
    return value;
}
void Database::setShowRandomNSFW(bool flag)
{
    setBoolProperty(flag, "SHOW_RANDOM_NSFW");
}
bool Database::getShowRandomNSFW() const
{
    return getBoolProperty("SHOW_RANDOM_NSFW", false);
}

void Database::setBlurNSFWPictures(bool flag)
{
    setBoolProperty(flag,"BLUR_NSFW");
}
bool Database::getBlurNSFWPictures() const
{
    return getBoolProperty("BLUR_NSFW", true);
}
void Database::setUseHWAccelerationForMedia(bool flag)
{
    setBoolProperty(flag,"MEDIA_HW_ACCEL");
}
bool Database::getUseHWAccelerationForMedia() const
{
    return getBoolProperty("MEDIA_HW_ACCEL", true);
}

void Database::setMediaAudioVolume(int volume)
{
    setIntProperty(volume,"VOLUME");
}
int Database::getMediaAudioVolume()
{
    return getIntProperty("VOLUME",100);
}
void Database::setAutoArangeWindowsGrid(bool flag)
{
    setBoolProperty(flag,"GRID_AUTO_ARRANGE");
}
bool Database::getAutoArangeWindowsGrid() const
{
    return getBoolProperty("GRID_AUTO_ARRANGE", false);
}

void Database::getMainWindowDimensions(int *x, int *y, int *width,int *height)
{
    std::unique_ptr<sqlite3_stmt,statement_finalizer> stmt;
    sqlite3_stmt *stmt_ptr;
    int rc = sqlite3_prepare_v2(db.get(),"SELECT NAME,PROP_VAL FROM PROPERTIES WHERE NAME='WINDOW_X' OR NAME='WINDOW_Y' "
                                         "OR NAME='WINDOW_WIDTH' OR NAME='WINDOW_HEIGHT'",-1,&stmt_ptr, nullptr);
    stmt.reset(stmt_ptr);
    DB_ERR_CHECK("Cannot find window dimensions");
    SDL_DisplayMode displayMode;
    SDL_GetDesktopDisplayMode(0, &displayMode);
    *width = 1280;
    *height = 720;

    *x = (displayMode.w - *width) / 2;
    *y = (displayMode.h - *height) / 2;
    while(sqlite3_step(stmt.get()) == SQLITE_ROW)
    {
       auto name = sqlite3_column_text(stmt.get(),0);
       auto val = sqlite3_column_int(stmt.get(),1);
       if(std::string("WINDOW_X") == std::string(reinterpret_cast<const char*>(name)))
       {
            *x = val;
       }
       else if(std::string("WINDOW_Y") == std::string(reinterpret_cast<const char*>(name)))
       {
            *y = val;
       }
       else if(std::string("WINDOW_WIDTH") == std::string(reinterpret_cast<const char*>(name)))
       {
            *width = val;
       }
       else if(std::string("WINDOW_HEIGHT") == std::string(reinterpret_cast<const char*>(name)))
       {
            *height = val;
       }
    }
}
void Database::setMainWindowDimensions(int x, int y, int width,int height)
{
    sqlite3_exec(db.get(),"DELETE FROM PROPERTIES WHERE NAME='WINDOW_X' OR NAME='WINDOW_Y' "
                          "OR NAME='WINDOW_WIDTH' OR NAME='WINDOW_HEIGHT'",nullptr,nullptr,nullptr);
    std::unique_ptr<sqlite3_stmt,statement_finalizer> stmt;
    sqlite3_stmt *stmt_ptr;
    int rc = sqlite3_prepare_v2(db.get(),"INSERT INTO PROPERTIES(NAME,PROP_VAL) VALUES(?,?)",-1,&stmt_ptr, nullptr);
    stmt.reset(stmt_ptr);
    DB_ERR_CHECK("Cannot insert window dimensions");

    rc = sqlite3_bind_text(stmt.get(),1,"WINDOW_X",-1,nullptr);
    DB_ERR_CHECK("Cannot bind values to window dims")
    rc = sqlite3_bind_int(stmt.get(),2,x);
    DB_ERR_CHECK("Cannot bind values to window dims");
    rc = sqlite3_step(stmt.get());
    DB_ERR_CHECK("Cannot insert window dims");
    sqlite3_reset(stmt.get());

    rc = sqlite3_bind_text(stmt.get(),1,"WINDOW_Y",-1,nullptr);
    DB_ERR_CHECK("Cannot bind values to window dims")
    rc = sqlite3_bind_int(stmt.get(),2,y);
    DB_ERR_CHECK("Cannot bind values to window dims");
    rc = sqlite3_step(stmt.get());
    DB_ERR_CHECK("Cannot insert window dims");
    sqlite3_reset(stmt.get());

    rc = sqlite3_bind_text(stmt.get(),1,"WINDOW_WIDTH",-1,nullptr);
    DB_ERR_CHECK("Cannot bind values to window dims")
    rc = sqlite3_bind_int(stmt.get(),2,width);
    DB_ERR_CHECK("Cannot bind values to window dims");
    rc = sqlite3_step(stmt.get());
    DB_ERR_CHECK("Cannot insert window dims");
    sqlite3_reset(stmt.get());

    rc = sqlite3_bind_text(stmt.get(),1,"WINDOW_HEIGHT",-1,nullptr);
    DB_ERR_CHECK("Cannot bind values to window dims")
    rc = sqlite3_bind_int(stmt.get(),2,height);
    DB_ERR_CHECK("Cannot bind values to window dims");
    rc = sqlite3_step(stmt.get());
    DB_ERR_CHECK("Cannot insert window dims");
}

std::optional<user> Database::getRegisteredUser() const
{
    std::unique_ptr<sqlite3_stmt,statement_finalizer> stmt;
    sqlite3_stmt *stmt_ptr;
    int rc = sqlite3_prepare_v2(db.get(),"SELECT USERNAME,PASSWORD,CLIENT_ID,SECRET,WEBSITE,APP_NAME FROM USER",-1,&stmt_ptr, nullptr);
    stmt.reset(stmt_ptr);
    DB_ERR_CHECK("Cannot find users");
    if(sqlite3_step(stmt.get()) != SQLITE_ROW)
    {
        return std::optional<user>();
    }
    std::string username(reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(),0)),
                         sqlite3_column_bytes(stmt.get(),0));
    std::string password(reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(),1)),
                         sqlite3_column_bytes(stmt.get(),1));
    std::string client_id(reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(),2)),
                         sqlite3_column_bytes(stmt.get(),2));
    std::string secret(reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(),3)),
                         sqlite3_column_bytes(stmt.get(),3));
    std::string website(reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(),4)),
                         sqlite3_column_bytes(stmt.get(),4));
    std::string app_name(reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(),5)),
                         sqlite3_column_bytes(stmt.get(),5));
    return std::make_optional<user>(username,password,client_id,secret,website,app_name);
}

void Database::setRegisteredUser(const user& registeredUser)
{
    sqlite3_exec(db.get(),"DELETE FROM USER",nullptr,nullptr,nullptr);
    std::unique_ptr<sqlite3_stmt,statement_finalizer> stmt;
    sqlite3_stmt *stmt_ptr;
    int rc = sqlite3_prepare_v2(db.get(),"INSERT INTO USER(USERNAME,PASSWORD,CLIENT_ID,SECRET,WEBSITE,APP_NAME) VALUES(?,?,?,?,?,?)",-1,&stmt_ptr, nullptr);
    stmt.reset(stmt_ptr);
    DB_ERR_CHECK("Cannot insert user");
    rc = sqlite3_bind_text(stmt.get(),1,registeredUser.username.c_str(),-1,nullptr);
    DB_ERR_CHECK("Cannot bind values to user");
    rc = sqlite3_bind_text(stmt.get(),2,registeredUser.password.c_str(),-1,nullptr);
    DB_ERR_CHECK("Cannot bind values to user");
    rc = sqlite3_bind_text(stmt.get(),3,registeredUser.client_id.c_str(),-1,nullptr);
    DB_ERR_CHECK("Cannot bind values to user");
    rc = sqlite3_bind_text(stmt.get(),4,registeredUser.secret.c_str(),-1,nullptr);
    DB_ERR_CHECK("Cannot bind values to user");
    rc = sqlite3_bind_text(stmt.get(),5,registeredUser.website.c_str(),-1,nullptr);
    DB_ERR_CHECK("Cannot bind values to user");
    rc = sqlite3_bind_text(stmt.get(),6,registeredUser.app_name.c_str(),-1,nullptr);
    DB_ERR_CHECK("Cannot bind values to user");
    rc = sqlite3_step(stmt.get());
    DB_ERR_CHECK("Cannot insert user");
}

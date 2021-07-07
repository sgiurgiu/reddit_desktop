#ifndef DATABASE_H
#define DATABASE_H

#include <stdexcept>
#include <memory>
#include <vector>

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
    std::vector<user> getRegisteredUsers() const;
    void addRegisteredUser(const user& registeredUser);
    void setLoggedInUser(const user& u);
    void getMainWindowDimensions(int *x, int *y, int *width,int *height);
    void setMainWindowDimensions(int x, int y, int width,int height);
    void setMediaAudioVolume(int volume);
    int getMediaAudioVolume();
    void setBlurNSFWPictures(bool flag);
    bool getBlurNSFWPictures() const;
    void setUseHWAccelerationForMedia(bool flag);
    bool getUseHWAccelerationForMedia() const;
    int getAutoRefreshTimeout() const;
    void setAutoRefreshTimeout(int value);
    void setShowRandomNSFW(bool flag);
    bool getShowRandomNSFW() const;
    void setAutoArangeWindowsGrid(bool flag);
    bool getAutoArangeWindowsGrid() const;
    void setUseYoutubeDownloader(bool flag);
    bool getUseYoutubeDownloader() const;
    std::vector<std::string> getMediaDomains() const;
    void addMediaDomains(const std::vector<std::string>& domains);
    void addMediaDomain(const std::string& domain);
    void removeMediaDomain(const std::string& domain);
private:
    void setBoolProperty(bool flag, const std::string& propName);
    bool getBoolProperty(const std::string& propName, bool defaultValue) const;
    void setIntProperty(int value, const std::string& propName);
    int getIntProperty(const std::string& propName, int defaultValue) const;
    int getSchemaVersion();
    void incrementSchemaVersion(int version);
private:
    std::unique_ptr<sqlite3,connection_deleter> db;
    static std::unique_ptr<Database> instance;
};

#endif // DATABASE_H

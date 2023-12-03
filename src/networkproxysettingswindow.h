#ifndef NETWORKPROXYSETTINGSWINDOW_H
#define NETWORKPROXYSETTINGSWINDOW_H

class NetworkProxySettingsWindow
{
public:
    NetworkProxySettingsWindow();
    bool showProxySettingsWindow();
    void setShowProxySettingsWindow(bool flag);
private:
    bool openWindow = false;
    char username[255] = { 0 };
    char password[255] = { 0 };
    const char* proxyType = nullptr;
    char host[255] = { 0 };
    char port[255] = { 0 };
    bool useProxy = false;
};

#endif // NETWORKPROXYSETTINGSWINDOW_H

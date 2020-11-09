#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H


class LoginWindow
{
public:
    LoginWindow();
    void setShowLoginWindow(bool flag);
    bool showLoginWindow();
private:
    bool showLoginInfoWindow = false;
    char username[255] = { 0 };
    char password[255] = { 0 };
    char clientId[255] = { 0 };
    char secret[255] = { 0 };
    char website[255] = { 0 };
    char appName[255] = { 0 };
    bool tested = false;

};

#endif // LOGINWINDOW_H

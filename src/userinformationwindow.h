#ifndef USERINFORMATIONWINDOW_H
#define USERINFORMATIONWINDOW_H


class UserInformationWindow
{
public:
    UserInformationWindow();
    void showUserInfoWindow();
    void shouldShowWindow(bool flag)
    {
        showWindow = flag;
    }
    bool isWindowShowing() const
    {
        return showWindow;
    }
private:
    bool showWindow = false;
};

#endif // USERINFORMATIONWINDOW_H

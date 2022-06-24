#ifndef USERINFORMATIONPROVIDER_H
#define USERINFORMATIONPROVIDER_H

#include <QObject>
#include <QNetworkReply>

class QNetworkAccessManager;
class AccessToken;
class UserInfo;

class UserInformationProvider : public QObject
{
    Q_OBJECT
public:
    explicit UserInformationProvider(QObject *parent = nullptr);
    Q_INVOKABLE void loadUserInformation(const AccessToken* const token);
signals:
    void success(UserInfo* userInfo);
    void failure(QString error);
private slots:
    void onNetworkAccessFinished();
    void onNetworkAccessError(QNetworkReply::NetworkError);
private:
    QNetworkAccessManager* networkAccessManager;

};

#endif // USERINFORMATIONPROVIDER_H

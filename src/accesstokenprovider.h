#ifndef ACCESSTOKENPROVIDER_H
#define ACCESSTOKENPROVIDER_H

#include <QObject>
#include <QNetworkReply>
#include "entities.h"

class QNetworkAccessManager;


class AccessToken : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString token READ getToken)
    Q_PROPERTY(QString tokenType READ getTokenType)
    Q_PROPERTY(QString scope READ getScope)
    Q_PROPERTY(int expires READ getExpires)

public:
    explicit AccessToken(QObject *parent = nullptr);
    QString getToken() const;
    QString getTokenType() const;
    QString getScope() const;
    int getExpires() const;

    void setToken(const QString&);
    void setTokenType(const QString&);
    void setScope(const QString&);
    void  setExpires(int);
private:
    QString token;
    QString tokenType;
    int expires = 0;
    QString scope;
};

class AccessTokenProvider : public QObject
{
    Q_OBJECT
public:
    explicit AccessTokenProvider(QObject *parent = nullptr);
    Q_INVOKABLE void refreshAccessToken();
signals:
    void success(AccessToken* token);
    void failure(QString error);
private slots:
    void onNetworkAccessFinished();
    void onNetworkAccessError(QNetworkReply::NetworkError);
private:
    void makeRequest(const user& u);
private:
    QNetworkAccessManager* _networkAccessManager;
};

#endif // ACCESSTOKENPROVIDER_H

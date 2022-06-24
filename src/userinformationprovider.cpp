#include "userinformationprovider.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>

#include "accesstokenprovider.h"
#include "entities/userinfo.h"

UserInformationProvider::UserInformationProvider(QObject *parent)
    : QObject{parent},
      networkAccessManager{QCoreApplication::instance()->property("NetworkAccessManager").value<QNetworkAccessManager*>()}
{
}
void UserInformationProvider::loadUserInformation(const AccessToken* const token)
{
    QNetworkRequest request;
    QUrl url("https://oauth.reddit.com/api/v1/me?raw_json=1");
    request.setUrl(url);
    request.setHeader(QNetworkRequest::KnownHeaders::UserAgentHeader,token->getUserAgent());
    request.setRawHeader("Authorization",QString("Bearer %1").arg(token->getToken()).toUtf8());
    request.setRawHeader("Accept","application/json");

    auto reply = networkAccessManager->get(request);
    connect(reply,&QNetworkReply::finished,this,&UserInformationProvider::onNetworkAccessFinished);
    connect(reply,&QNetworkReply::errorOccurred,this,&UserInformationProvider::onNetworkAccessError);
}
void UserInformationProvider::onNetworkAccessFinished()
{
    QSharedPointer<QNetworkReply> reply ((QNetworkReply*)sender(), &QObject::deleteLater);
    const QByteArray replyData = reply->readAll();
    if ( reply->error() == QNetworkReply::NoError )
    {
        auto jsonDoc = QJsonDocument::fromJson(replyData);
        if(!jsonDoc.isObject())
        {
            emit failure("Invalid response from reddit api");
            return;
        }
        auto json = jsonDoc.object();

        emit success(new UserInfo(json,this));
    }
}
void UserInformationProvider::onNetworkAccessError(QNetworkReply::NetworkError)
{
    auto reply = (QNetworkReply*)sender();
    emit failure(reply->errorString());
}

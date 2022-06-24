#include "accesstokenprovider.h"
#include "database.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QCoreApplication>

AccessTokenProvider::AccessTokenProvider(QObject *parent)
    : QObject{parent},networkAccessManager{QCoreApplication::instance()->property("NetworkAccessManager").value<QNetworkAccessManager*>()}
{    
}

void AccessTokenProvider::refreshAccessToken()
{
    auto db = Database::getInstance();
    auto u = db->getLastLoggedInUser();
    if(!u)
    {
        emit failure("No user found");
    }
    else
    {
        makeRequest(u.value());
    }
}
void AccessTokenProvider::makeRequest(const user& u)
{
    //"api.reddit.com","oauth.reddit.com"
    QNetworkRequest request;
    QUrl url("https://api.reddit.com/api/v1/access_token");
    request.setUrl(url);
    request.setHeader(QNetworkRequest::KnownHeaders::UserAgentHeader,make_user_agent(u).c_str());
    QString authorization = QString("%1:%2").arg(QString::fromStdString(u.client_id),QString::fromStdString(u.secret)).toUtf8().toBase64();
    request.setRawHeader("Authorization",QString("Basic %1").arg(authorization).toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
    QUrlQuery postData;
    postData.addQueryItem("grant_type", "password");
    postData.addQueryItem("username", QString::fromStdString(u.username));
    postData.addQueryItem("password", QString::fromStdString(u.password));

    auto reply = networkAccessManager->post(request,postData.toString(QUrl::FullyEncoded).toUtf8());
    reply->setProperty("user_agent",QVariant::fromValue(QString::fromStdString(make_user_agent(u))));
    connect(reply,&QNetworkReply::finished,this,&AccessTokenProvider::onNetworkAccessFinished);
    connect(reply,&QNetworkReply::errorOccurred,this,&AccessTokenProvider::onNetworkAccessError);
}

void AccessTokenProvider::onNetworkAccessFinished()
{
    auto reply = (QNetworkReply*)sender();
    const QByteArray replyData = reply->readAll();
    if ( reply->error() == QNetworkReply::NoError )
    {
        auto json = nlohmann::json::parse(replyData.toStdString());
        AccessToken* token = new AccessToken(this);
        token->setUserAgent(reply->property("user_agent").toString());
        if(json.contains("access_token") && json["access_token"].is_string())
        {
            token->setToken(QString::fromStdString(json["access_token"].get<std::string>()));
        }
        if(json.contains("token_type") && json["token_type"].is_string())
        {
            token->setTokenType(QString::fromStdString(json["token_type"].get<std::string>()));
        }
        if(json.contains("expires_in") && json["expires_in"].is_number())
        {
            token->setExpires(json["expires_in"].get<int>());
        }
        if(json.contains("scope") && json["scope"].is_string())
        {
            token->setScope(QString::fromStdString(json["scope"].get<std::string>()));
        }
        if (json.contains("error") && json["error"].is_string())
        {
            emit failure(QString::fromStdString(json["error"].get<std::string>()));
        }
        else
        {            
            emit success(token);
        }
    }

    reply->deleteLater();
}
void AccessTokenProvider::onNetworkAccessError(QNetworkReply::NetworkError)
{
    auto reply = (QNetworkReply*)sender();
    emit failure(reply->errorString());
}

AccessToken::AccessToken(QObject *parent): QObject{parent}
{
}

QString AccessToken::getToken() const
{
    return token;
}
QString AccessToken::getTokenType() const
{
    return tokenType;
}
QString AccessToken::getScope() const
{
    return scope;
}
int AccessToken::getExpires() const
{
    return expires;
}
QString AccessToken::getUserAgent() const
{
    return userAgent;
}
void AccessToken::setUserAgent(const QString& u)
{
    this->userAgent = u;
}
void AccessToken::setToken(const QString& t)
{
    this->token = t;
}
void AccessToken::setTokenType(const QString& t)
{
    this->tokenType = t;
}
void AccessToken::setScope(const QString& s)
{
    this->scope = s;
}
void  AccessToken::setExpires(int e)
{
    this->expires = e;
}



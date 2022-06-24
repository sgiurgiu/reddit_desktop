#include "postsprovider.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "accesstokenprovider.h"
#include "entities/post.h"

PostsProvider::PostsProvider(QObject *parent)
    : QObject{parent},
      networkAccessManager{QCoreApplication::instance()->property("NetworkAccessManager").value<QNetworkAccessManager*>()}
{
}
void PostsProvider::loadPosts(const QString& path, const AccessToken* const token)
{
    QNetworkRequest request;
    QString requestPath = path;
    if(!requestPath.startsWith("r/") && !requestPath.startsWith("/r/") &&
        (!requestPath.startsWith("/user/") && !requestPath.contains("/m/")))
    {
        requestPath.insert(0, "/r/");
    }
    if(!requestPath.startsWith('/'))
    {
        requestPath.insert(0,'/');
    }
    if(requestPath.contains('?'))
    {
        requestPath.append("&raw_json=1");
    }
    else
    {
        requestPath.append("?raw_json=1");
    }

    QUrl url("https://oauth.reddit.com"+requestPath);
    request.setUrl(url);
    request.setHeader(QNetworkRequest::KnownHeaders::UserAgentHeader,token->getUserAgent());
    request.setRawHeader("Authorization",QString("Bearer %1").arg(token->getToken()).toUtf8());
    request.setRawHeader("Accept","application/json");

    auto reply = networkAccessManager->get(request);
    connect(reply,&QNetworkReply::finished,this,&PostsProvider::onNetworkAccessFinished);
    connect(reply,&QNetworkReply::errorOccurred,this,&PostsProvider::onNetworkAccessError);
}
void PostsProvider::onNetworkAccessFinished()
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
        if(json.contains("data") && json["data"].isObject())
        {
            auto jsonData = json["data"].toObject();
            if(jsonData.contains("children") && jsonData["children"].isArray())
            {
                QList<Post*> postsList;
                auto jsonChildren = jsonData["children"].toArray();
                for(const auto& child: jsonChildren)
                {
                    auto childObj = child.toObject();
                    postsList.append(new Post(childObj["data"].toObject(),this));
                }
                emit postsLoaded(std::move(postsList));
            }
        }
    }
}
void PostsProvider::onNetworkAccessError(QNetworkReply::NetworkError)
{
    auto reply = (QNetworkReply*)sender();
    emit failure(reply->errorString());
}

#include "subscribedsubredditsprovider.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "accesstokenprovider.h"
#include "entities/subreddit.h"

SubscribedSubredditsProvider::SubscribedSubredditsProvider(QObject *parent)
    : QObject{parent},
      networkAccessManager{QCoreApplication::instance()->property("NetworkAccessManager").value<QNetworkAccessManager*>()}
{

}

void SubscribedSubredditsProvider::loadSubscribedSubreddits(const AccessToken* const token)
{
    QNetworkRequest subredditsRequest;
    QUrl subredditsUrl("https://oauth.reddit.com/subreddits/mine/subscriber?show=all&limit=100&raw_json=1");
    subredditsRequest.setUrl(subredditsUrl);
    subredditsRequest.setHeader(QNetworkRequest::KnownHeaders::UserAgentHeader,token->getUserAgent());
    subredditsRequest.setRawHeader("Authorization",QString("Bearer %1").arg(token->getToken()).toUtf8());
    subredditsRequest.setRawHeader("Accept","application/json");

    QNetworkRequest multisRequest;
    QUrl multisUrl("https://oauth.reddit.com/api/multi/mine?raw_json=1");
    multisRequest.setUrl(multisUrl);
    multisRequest.setHeader(QNetworkRequest::KnownHeaders::UserAgentHeader,token->getUserAgent());
    multisRequest.setRawHeader("Authorization",QString("Bearer %1").arg(token->getToken()).toUtf8());
    multisRequest.setRawHeader("Accept","application/json");


    auto subredditsReply = networkAccessManager->get(subredditsRequest);
    connect(subredditsReply,&QNetworkReply::finished,this,&SubscribedSubredditsProvider::onSubredditsNetworkAccessFinished);
    connect(subredditsReply,&QNetworkReply::errorOccurred,this,&SubscribedSubredditsProvider::onNetworkAccessError);

    auto multisReply = networkAccessManager->get(multisRequest);
    connect(multisReply,&QNetworkReply::finished,this,&SubscribedSubredditsProvider::onMultisNetworkAccessFinished);
    connect(multisReply,&QNetworkReply::errorOccurred,this,&SubscribedSubredditsProvider::onNetworkAccessError);
}

void SubscribedSubredditsProvider::onSubredditsNetworkAccessFinished()
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
                QList<Subreddit*> subredditsList;
                auto jsonChildren = jsonData["children"].toArray();
                for(const auto& child: jsonChildren)
                {
                    auto childObj = child.toObject();
                    subredditsList.append(new Subreddit(childObj["data"].toObject(),this));
                }
                emit subredditsLoaded(std::move(subredditsList));
            }
        }

    }
}
void SubscribedSubredditsProvider::onMultisNetworkAccessFinished()
{
    QSharedPointer<QNetworkReply> reply ((QNetworkReply*)sender(), &QObject::deleteLater);
    const QByteArray replyData = reply->readAll();
    if ( reply->error() == QNetworkReply::NoError )
    {
        auto jsonDoc = QJsonDocument::fromJson(replyData);
        if(!jsonDoc.isArray())
        {
            emit failure("Invalid response from reddit api");
            return;
        }
        auto json = jsonDoc.array();
        QList<Multireddit*> subredditsList;
        for(const auto& child: json)
        {
            auto childObj = child.toObject();
            subredditsList.append(new Multireddit(childObj["data"].toObject(),this));
        }
        emit multisLoaded(std::move(subredditsList));

    }
}
void SubscribedSubredditsProvider::onNetworkAccessError(QNetworkReply::NetworkError)
{
    auto reply = (QNetworkReply*)sender();
    emit failure(reply->errorString());
}

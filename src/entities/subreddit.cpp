#include "subreddit.h"
#include <QJsonObject>
#include <QJsonArray>
#include "utils.h"

SubredditBase::SubredditBase(QObject *parent)
    : QObject{parent}
{
}
Subreddit::Subreddit(QObject *parent)
    : SubredditBase{parent}
{
}
Multireddit::Multireddit(QObject *parent)
    : SubredditBase{parent}
{
}
SubredditBase::SubredditBase(const QJsonObject& json,QObject *parent)
    : QObject{parent}
{
    if(json.contains("name") && json["name"].isString())
    {
        name =json["name"].toString();
    }
    if(json.contains("created_utc") && json["created_utc"].isDouble())
    {
        createdAt = json["created_utc"].toDouble();
        humanReadableTimeDifference = QString::fromStdString(Utils::getHumanReadableTimeAgo(createdAt));
    }
    if(json.contains("description") && json["description"].isString())
    {
        description =json["description"].toString();
    }
    if(json.contains("display_name") && json["display_name"].isString())
    {
        displayName =json["display_name"].toString();
    }
    if(json.contains("subscribers") && json["subscribers"].isDouble())
    {
        subscribers = json["subscribers"].toDouble();
    }
    if(json.contains("over18") && json["over18"].isBool())
    {
        over18 = json["over18"].toBool();
    }

}
Subreddit::Subreddit(const QJsonObject& json,QObject *parent)
    : SubredditBase{json,parent}
{
    if(json.contains("id") && json["id"].isString())
    {
        id = json["id"].toString();
    }
    if(json.contains("display_name_prefixed") && json["display_name_prefixed"].isString())
    {
        displayNamePrefixed =json["display_name_prefixed"].toString();
    }
    if(json.contains("title") && json["title"].isString())
    {
        title =json["title"].toString();
    }
    if(json.contains("submit_text") && json["submit_text"].isString())
    {
        submitText = json["submit_text"].toString();
    }
    if(json.contains("url") && json["url"].isString())
    {
        url = json["url"].toString();
    }
    if(json.contains("icon_img") && json["icon_img"].isString())
    {
        iconImage = json["icon_img"].toString();
    }
    if(json.contains("header_img") && json["header_img"].isString())
    {
        headerImage = json["header_img"].toString();
    }
    if(json.contains("quarantine") && json["quarantine"].isBool())
    {
        quarantine = json["quarantine"].toBool();
    }
    if(json.contains("user_is_banned") && json["user_is_banned"].isBool())
    {
        userIsBanned = json["user_is_banned"].toBool();
    }
    if(json.contains("user_is_moderator") && json["user_is_moderator"].isBool())
    {
        userIsModerator = json["user_is_moderator"].toBool();
    }
    if(json.contains("user_is_muted") && json["user_is_muted"].isBool())
    {
        userIsMuted = json["user_is_muted"].toBool();
    }
    if(json.contains("user_is_subscriber") && json["user_is_subscriber"].isBool())
    {
        userIsSubscriber = json["user_is_subscriber"].toBool();
    }
    if(json.contains("header_size") && json["header_size"].isArray() &&
           json["header_size"].toArray().size() == 2 )
    {
        headerSize.first = json["header_size"][0].toDouble();
        headerSize.second = json["header_size"][1].toDouble();
    }
    if(json.contains("icon_size") && json["icon_size"].isArray() &&
           json["icon_size"].toArray().size() == 2 )
    {
        iconSize.first = json["icon_size"][0].toDouble();
        iconSize.second = json["icon_size"][1].toDouble();
    }
}
Multireddit::Multireddit(const QJsonObject& json,QObject *parent)
    : SubredditBase{json,parent}
{
    if(json.contains("owner") && json["owner"].isString())
    {
        owner =json["owner"].toString();
    }
    if(json.contains("owner_id") && json["owner_id"].isString())
    {
        ownerId =json["owner_id"].toString();
    }
    if(json.contains("can_edit") && json["can_edit"].isBool())
    {
        canEdit = json["can_edit"].toBool();
    }
    if(json.contains("path") && json["path"].isString())
    {
        path =json["path"].toString();
    }
}

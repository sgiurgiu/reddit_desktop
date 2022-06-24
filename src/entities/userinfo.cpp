#include "userinfo.h"
#include <QJsonObject>
#include "utils.h"

UserInfo::UserInfo(QObject *parent)
    : QObject{parent}
{

}

UserInfo::UserInfo(const QJsonObject& json, QObject *parent)
    :QObject{parent}
{
    if(json.contains("id") && json["id"].isString())
    {
        id = json["id"].toString();
    }
    if(json.contains("name") && json["name"].isString())
    {
        name =json["name"].toString();
    }
    if(json.contains("oauth_client_id") && json["oauth_client_id"].isString())
    {
        oauthClientId =json["oauth_client_id"].toString();
    }
    if(json.contains("created_utc") && json["created_utc"].isDouble())
    {
        createdAt = json["created_utc"].toDouble();
        humanReadableTimeDifference = QString::fromStdString(Utils::getHumanReadableTimeAgo(createdAt));
    }
    if(json.contains("inbox_count") && json["inbox_count"].isDouble())
    {
        inboxCount = json["inbox_count"].toInt();
    }
    if(json.contains("comment_karma") && json["comment_karma"].isDouble())
    {
        commentKarma = json["comment_karma"].toDouble();
        humanCommentKarma = QString::fromStdString(Utils::getHumanReadableNumber(commentKarma));
    }
    if(json.contains("total_karma") && json["total_karma"].isDouble())
    {
        totalKarma = json["total_karma"].toDouble();
        humanTotalKarma = QString::fromStdString(Utils::getHumanReadableNumber(totalKarma));
    }
    if(json.contains("link_karma") && json["link_karma"].isDouble())
    {
        linkKarma = json["link_karma"].toDouble();
        humanLinkKarma = QString::fromStdString(Utils::getHumanReadableNumber(linkKarma));
    }
    if(json.contains("has_mail") && json["has_mail"].isBool())
    {
        hasMail = json["has_mail"].toBool();
    }
    if(json.contains("has_mod_mail") && json["has_mod_mail"].isBool())
    {
        hasModMail = json["has_mod_mail"].toBool();
    }
    if(json.contains("gold_creddits") && json["gold_creddits"].isDouble())
    {
        goldCreddits = json["gold_creddits"].toBool();
    }
    if(json.contains("is_suspended") && json["is_suspended"].isBool())
    {
        isSuspended = json["is_suspended"].toBool();
    }
    if(json.contains("is_mod") && json["is_mod"].isBool())
    {
        isMod = json["is_mod"].toBool();
    }
    if(json.contains("is_gold") && json["is_gold"].isBool())
    {
        isGold = json["is_gold"].toBool();
    }
    if(json.contains("coins") && json["coins"].isDouble())
    {
        coins = json["coins"].toDouble();
    }

}

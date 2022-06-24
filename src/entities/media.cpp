#include "media.h"
#include <QJsonObject>
#include <QJsonArray>
#include "utils.h"

Media::Media(QObject *parent)
    : QObject{parent}
{

}
Media::Media(const QJsonObject& json, QObject *parent)
    : QObject{parent}
{
    if(json.contains("reddit_video") && json["reddit_video"].isObject())
    {
        redditVideo = new RedditVideo(json["reddit_video"].toObject(),this);
    }
    if(json.contains("oembed") && json["oembed"].isObject())
    {
        oembed = new OEmbed(json["oembed"].toObject(),this);
    }
    if(json.contains("type") && json["type"].isString())
    {
        type = json["type"].toString();
    }

}
OEmbed::OEmbed(QObject *parent)
    : QObject{parent}
{

}
OEmbed::OEmbed(const QJsonObject& json, QObject *parent)
    : QObject{parent}
{
    if(json.contains("height") && json["height"].isDouble())
    {
        height = json["height"].toDouble();
    }
    if(json.contains("width") && json["width"].isDouble())
    {
        width = json["width"].toDouble();
    }
    if(json.contains("thumbnail_height") && json["thumbnail_height"].isDouble())
    {
        thumbnailHeight = json["thumbnail_height"].toDouble();
    }
    if(json.contains("thumbnail_width") && json["thumbnail_width"].isDouble())
    {
        thumbnailWidth = json["thumbnail_width"].toDouble();
    }
    if(json.contains("provider_url") && json["provider_url"].isString())
    {
        providerUrl = json["provider_url"].toString();
    }
    if(json.contains("html") && json["html"].isString())
    {
        html = json["html"].toString();
    }
    if(json.contains("thumbnail_url") && json["thumbnail_url"].isString())
    {
        thumbnailUrl = json["thumbnail_url"].toString();
    }
    if(json.contains("title") && json["title"].isString())
    {
        title = json["title"].toString();
    }
    if(json.contains("provider_name") && json["provider_name"].isString())
    {
        providerName = json["provider_name"].toString();
    }
    if(json.contains("type") && json["type"].isString())
    {
        type = json["type"].toString();
    }
}
RedditVideo::RedditVideo(QObject *parent)
    : QObject{parent}
{

}
RedditVideo::RedditVideo(const QJsonObject& json, QObject *parent)
    : QObject{parent}
{
    if(json.contains("bitrate_kbps") && json["bitrate_kbps"].isDouble())
    {
        bitrate = json["bitrate_kbps"].toDouble();
    }
    if(json.contains("height") && json["height"].isDouble())
    {
        height = json["height"].toDouble();
    }
    if(json.contains("width") && json["width"].isDouble())
    {
        width = json["width"].toDouble();
    }
    if(json.contains("duration") && json["duration"].isDouble())
    {
        duration = json["duration"].toDouble();
    }
    if(json.contains("fallback_url") && json["fallback_url"].isString())
    {
        fallbackUrl = json["fallback_url"].toString();
    }
    if(json.contains("scrubber_media_url") && json["scrubber_media_url"].isString())
    {
        scrubberMediaUrl = json["scrubber_media_url"].toString();
    }
    if(json.contains("dash_url") && json["dash_url"].isString())
    {
        dashUrl = json["dash_url"].toString();
    }
    if(json.contains("hls_url") && json["hls_url"].isString())
    {
        hlsUrl = json["hls_url"].toString();
    }
    if(json.contains("transcoding_status") && json["transcoding_status"].isString())
    {
        transcodingStatus = json["transcoding_status"].toString();
    }
    if(json.contains("is_gif") && json["is_gif"].isBool())
    {
        isGif = json["is_gif"].toBool();
    }
}

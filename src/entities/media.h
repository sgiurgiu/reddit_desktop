#ifndef MEDIA_H
#define MEDIA_H

#include <QObject>
#include <QString>
#include <QList>

class QJsonObject;


class RedditVideo : public QObject
{
    Q_OBJECT
public:
    explicit RedditVideo(QObject *parent = nullptr);
    explicit RedditVideo(const QJsonObject& json, QObject *parent = nullptr);

private:
    int bitrate;
    int height;
    int width;
    int64_t duration;
    QString fallbackUrl;
    QString scrubberMediaUrl;
    QString dashUrl;
    QString hlsUrl;
    bool isGif;
    QString transcodingStatus;
};

class OEmbed : public QObject
{
    Q_OBJECT
public:
    explicit OEmbed(QObject *parent = nullptr);
    explicit OEmbed(const QJsonObject& json, QObject *parent = nullptr);
private:
    int height;
    int width;
    int thumbnailHeight;
    int thumbnailWidth;
    QString providerUrl;
    QString thumbnailUrl;
    QString title;
    QString html;
    QString providerName;
    QString type;
};

class Media : public QObject
{
    Q_OBJECT
public:
    explicit Media(QObject *parent = nullptr);
    explicit Media(const QJsonObject& json, QObject *parent = nullptr);
    QString getType() const
    {
        return type;
    }
    RedditVideo* getRedditVideo() const
    {
        return redditVideo;
    }
    OEmbed* getOEmbed() const
    {
        return oembed;
    }
private:
    QString type;
    RedditVideo* redditVideo = nullptr;
    OEmbed* oembed = nullptr;

};

#endif // MEDIA_H

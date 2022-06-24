#ifndef POST_H
#define POST_H

#include <QObject>
#include <QString>
#include <QList>
#include <QMap>
#include <QtQml/qqmlregistration.h>

class QJsonObject;
class Image;
class ImageVariant;
class ImagePreview;
class Media;
class PostGalleryItem;
class Award;
class FlairRichText;

class Post : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString title READ getTitle NOTIFY titleChanged)
    Q_PROPERTY(QString selfText READ getSelfText  NOTIFY selfTextChanged)
    Q_PROPERTY(int ups READ getUps  NOTIFY upsChanged)
    Q_PROPERTY(int downs READ getDowns  NOTIFY downsChanged)
    Q_PROPERTY(int score READ getScore NOTIFY scoreChanged)
    Q_PROPERTY(QString thumbnail READ getThumbnail NOTIFY thumbnailChanged)
    Q_PROPERTY(QString humanReadableTimeDifference READ getHumanReadableTimeDifference NOTIFY humanReadableTimeDifferenceChanged)
    Q_PROPERTY(QString humanScore READ getHumanScore NOTIFY humanScoreChanged)
    Q_PROPERTY(QString subreddit READ getSubreddit NOTIFY subredditChanged)
    Q_PROPERTY(QString domain READ getDomain NOTIFY domainChanged)
    Q_PROPERTY(QString author READ getAuthor NOTIFY authorChanged)

    QML_ELEMENT

public:
    explicit Post(QObject *parent = nullptr);
    explicit Post(const QJsonObject& json, QObject *parent = nullptr);
    ~Post();

    enum class Voted
    {
        DownVoted = -1,
        NotVoted = 0,
        UpVoted = 1
    };
    Q_ENUM(Voted)

    QString getId() const
    {
        return id;
    }
    QString getTitle() const
    {
        return title;
    }
    QString getSelfText() const
    {
        return selfText;
    }
    int getUps() const
    {
        return ups;
    }
    int getDowns() const
    {
        return downs;
    }
    int getScore() const
    {
        return score;
    }
    QString getThumbnail() const
    {
        return thumbnail;
    }
    QString getHumanReadableTimeDifference() const
    {
        return humanReadableTimeDifference;
    }
    QString getHumanScore() const
    {
        return humanScore;
    }
    QString getSubreddit() const
    {
        return subreddit;
    }
    QString getDomain() const
    {
        return domain;
    }
    QString getAuthor() const
    {
        return author;
    }

signals:
    void titleChanged(QString);
    void selfTextChanged(QString);
    void upsChanged(int);
    void downsChanged(int);
    void scoreChanged(int);
    void thumbnailChanged(QString);
    void humanReadableTimeDifferenceChanged(QString);
    void humanScoreChanged(QString);
    void subredditChanged(QString);
    void domainChanged(QString);
    void authorChanged(QString);
private:
    QString name;
    QString id;
    QString title;
    QString selfText;
    int ups = 0;
    int downs = 0;
    bool isVideo = false;
    bool isSelf = false;
    bool isMeta = false;
    bool isOriginalContent = false;
    bool isRedditMediaDomain = false;
    bool isCrossPostable = false;
    QString thumbnail;
    uint64_t createdAt;
    QString humanReadableTimeDifference;
    int commentsCount = 0;
    QString commentsText;
    QString subreddit;
    QString subredditName;
    QString subredditId;
    int score = 0;
    QString humanScore;
    QString url;
    QString urlOverridenByDest;
    QList<ImagePreview*> previews;
    QString authorFullName;
    QString author;
    QString domain;
    QString postHint;
    Media* postMedia = nullptr;
    bool isGallery = false;
    QList<PostGalleryItem*> gallery;
    bool over18 = false;
    bool locked = false;
    bool clicked = false;
    bool visited = false;
    Voted voted = Voted::NotVoted;
    int gilded = 0;
    int totalAwardsReceived = 0;
    QList<Award*> allAwardings;
    QMap<QString,int> gildings;
    QString linkFlairType;
    QString linkFlairText;
    QString linkFlairTemplateId;
    QList<FlairRichText*> linkFlairsRichText;
    QString linkFlairBackgroundColor;
    QString linkFlairCSSClass;
    QString linkFlairTextColor;
    bool allow_live_comments = false;
    //this is an object
   // std::string link_rich_text;
};

#endif // POST_H

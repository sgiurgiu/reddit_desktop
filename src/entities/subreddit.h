#ifndef SUBREDDIT_H
#define SUBREDDIT_H

#include <QObject>
class QJsonObject;

class SubredditBase : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ getName NOTIFY nameChanged)
    Q_PROPERTY(QString displayName READ getDisplayName NOTIFY displayNameChanged)
public:
    explicit SubredditBase(QObject *parent = nullptr);
    explicit SubredditBase(const QJsonObject& json, QObject *parent = nullptr);
    virtual ~SubredditBase() = default;
    QString getName() const
    {
        return name;
    }
    QString getDisplayName() const
    {
        return displayName;
    }
    virtual bool isMulti() const = 0;
    virtual QString getTargetUrl() const = 0;
signals:
    void nameChanged();
    void displayNameChanged();
private:
    QString name;
    uint64_t createdAt;
    QString humanReadableTimeDifference;
    QString description;
    QString displayName;
    bool over18 = false;
    int64_t subscribers = 0;
};


class Subreddit : public SubredditBase
{
    Q_OBJECT
public:
    explicit Subreddit(QObject *parent = nullptr);
    explicit Subreddit(const QJsonObject& json, QObject *parent = nullptr);
    virtual bool isMulti() const override
    {
        return false;
    }
    virtual QString getTargetUrl() const override
    {
        return displayNamePrefixed;
    }
private:
    QString id;
    QString displayNamePrefixed;
    QString submitText;
    QString url;
    QString title;
    bool userIsBanned = false;
    bool userIsModerator = false;
    bool userIsSubscriber = false;
    bool userIsMuted = false;
    bool quarantine = false;
    QString iconImage;
    QString headerImage;
    std::pair<int,int> iconSize;
    std::pair<int,int> headerSize;
};

class Multireddit : public SubredditBase
{
    Q_OBJECT
public:
    explicit Multireddit(QObject *parent = nullptr);
    explicit Multireddit(const QJsonObject& json, QObject *parent = nullptr);
    virtual bool isMulti() const override
    {
        return true;
    }
    virtual QString getTargetUrl() const override
    {
        return path;
    }
private:
    QString owner;
    QString ownerId;
    bool canEdit = false;
    QString path;
};


#endif // SUBREDDIT_H

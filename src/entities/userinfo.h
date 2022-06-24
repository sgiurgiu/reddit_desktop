#ifndef USERINFO_H
#define USERINFO_H

#include <QObject>
class QJsonObject;

class UserInfo : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ getName NOTIFY nameChanged)
    Q_PROPERTY(QString humanLinkKarma READ getHumanLinkKarma NOTIFY humanLinkKarmaChanged)
    Q_PROPERTY(QString humanCommentKarma READ getHumanCommentKarma NOTIFY humanCommentKarmaChanged)

public:
    explicit UserInfo(QObject *parent = nullptr);
    explicit UserInfo(const QJsonObject& json, QObject *parent = nullptr);
    QString getName() const
    {
        return name;
    }
    QString getHumanLinkKarma() const
    {
        return humanLinkKarma;
    }
    QString getHumanCommentKarma() const
    {
        return humanCommentKarma;
    }
signals:
    void nameChanged();
    void humanLinkKarmaChanged();
    void humanCommentKarmaChanged();
private:
    QString name;
    QString id;
    QString oauthClientId;
    uint64_t createdAt;
    QString humanReadableTimeDifference;
    int inboxCount = 0;
    int64_t commentKarma = 0;
    int64_t totalKarma = 0;
    int64_t linkKarma = 0;
    QString humanCommentKarma;
    QString humanTotalKarma;
    QString humanLinkKarma;
    bool hasMail = false;
    bool hasModMail = false;
    int64_t goldCreddits = 0;
    bool isSuspended = false;
    bool isMod = false;
    bool isGold = false;
    int64_t coins = 0;
};

#endif // USERINFO_H

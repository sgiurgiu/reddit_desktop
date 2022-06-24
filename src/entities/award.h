#ifndef AWARD_H
#define AWARD_H

#include <QObject>
#include <QString>
#include <QList>

class QJsonObject;
class Image;

class Award : public QObject
{
    Q_OBJECT
public:
    explicit Award(QObject *parent = nullptr);
    explicit Award(const QJsonObject& json, QObject *parent = nullptr);
    QString getId() const
    {
        return id;
    }
    QString getName() const
    {
        return name;
    }
    QString getSubredditId() const
    {
        return subredditId;
    }
    QString getDescription() const
    {
        return description;
    }
    QString getAwardSubType() const
    {
        return awardSubType;
    }
    QString getAwardType() const
    {
        return awardType;
    }
    int getDaysOfPremium() const
    {
        return daysOfPremium;
    }
    int getCount() const
    {
        return count;
    }
    int getCoinPrice() const
    {
        return coinPrice;
    }
    bool isNew() const
    {
        return _new;
    }
    Image* getIcon() const
    {
        return icon;
    }
    Image* getStaticIcon() const
    {
        return staticIcon;
    }
    QList<Image*> getResizedIcons() const
    {
        return resizedIcons;
    }
    QList<Image*> getResizedStaticIcons() const
    {
        return resizedStaticIcons;
    }
private:
    QString id;
    QString name;
    QString subredditId;
    bool _new = false;
    Image* icon;
    Image* staticIcon;
    int daysOfPremium;
    QList<Image*> resizedIcons;
    QString description;
    int count = 0;
    QList<Image*> resizedStaticIcons;
    QString awardSubType;
    QString awardType;
    int coinPrice = 0;

};

#endif // AWARD_H

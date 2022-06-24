#ifndef POSTGALLERYITEM_H
#define POSTGALLERYITEM_H

#include <QObject>
#include <QString>
#include <QList>

class QJsonObject;

class PostGalleryItem : public QObject
{
    Q_OBJECT
public:
    explicit PostGalleryItem(QObject *parent = nullptr);
    explicit PostGalleryItem(const QJsonObject& json, QObject *parent = nullptr);
    void setUrl(const QString& url);
    QString getUrl() const
    {
        return url;
    }
    uint64_t getId() const
    {
        return id;
    }
    QString getMediaId() const
    {
        return mediaId;
    }    
private:
    QString url;
    uint64_t id = 0;
    QString mediaId;
};

#endif // POSTGALLERYITEM_H

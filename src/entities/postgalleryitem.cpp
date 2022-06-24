#include "postgalleryitem.h"
#include <QJsonObject>

PostGalleryItem::PostGalleryItem(QObject *parent)
    : QObject{parent}
{

}
PostGalleryItem::PostGalleryItem(const QJsonObject& json, QObject *parent)
{
    if(json.contains("id") && json["id"].isDouble())
    {
        id = json["id"].toDouble();
    }
    if(json.contains("media_id") && json["media_id"].isString())
    {
        mediaId = json["media_id"].toString();
    }
}

void PostGalleryItem::setUrl(const QString& url)
{
    this->url = url;
}

#include "image.h"
#include <QJsonObject>
#include <QJsonArray>
#include "utils.h"

Image::Image(QObject *parent)
    : QObject{parent}
{

}
Image::Image(const QJsonObject& json, QObject *parent)
    : QObject{parent}
{
    if(json.contains("url") && json["url"].isString())
    {
        url = json["url"].toString();
    }
    if(json.contains("width") && json["width"].isDouble())
    {
        width = json["width"].toDouble();
    }
    if(json.contains("height") && json["height"].isDouble())
    {
        height = json["height"].toDouble();
    }
}
void Image::setUrl(const QString& url)
{
    this->url = url;
}
void Image::setWidth(int w)
{
    this->width = w;
}
void Image::setHeight(int h)
{
    this->height = h;
}

ImageVariant::ImageVariant(QObject *parent)
    : QObject{parent}
{

}
ImageVariant::ImageVariant(const QString& kind, const QJsonObject& json, QObject *parent)
    : QObject{parent}, kind{kind}
{
    if (json.contains("source") && json["source"].isObject())
    {
        source = new Image(json["source"].toObject(),this);
    }
    if (json.contains("resolutions") && json["resolutions"].isArray())
    {
        for (const auto& res : json["resolutions"].toArray()) {
            resolutions.append(new Image(res.toObject(),this));
        }
    }
}
ImagePreview::ImagePreview(QObject *parent)
    : QObject{parent}
{
}
ImagePreview::ImagePreview(const QJsonObject& json, QObject* parent)
    : QObject{parent}
{
    if(json.contains("source") && json["source"].isObject())
    {
        source = new Image(json["source"].toObject(),this);
    }
    if(json.contains("resolutions") && json["resolutions"].isArray())
    {
        for(const auto& res : json["resolutions"].toArray()) {
            resolutions.append(new Image(res.toObject()));
        }
    }
    if (json.contains("variants") && json["variants"].isObject())
    {
        auto variantsObject = json["variants"].toObject();
        for (const auto& key : variantsObject.keys())
        {
            variants.append(new ImageVariant(key,variantsObject.value(key).toObject(),this));
        }
    }
}

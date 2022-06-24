#include "award.h"
#include <QJsonObject>
#include <QJsonArray>
#include "utils.h"
#include "image.h"
#include <tuple>

Award::Award(QObject *parent)
    : QObject{parent}
{

}

Award::Award(const QJsonObject& json, QObject *parent)
    : QObject{parent}
{
    id = json["id"].toString();
    name = json["name"].toString();
    icon = new Image(this);
    icon->setUrl(json["icon_url"].toString());
    icon->setWidth(json["icon_width"].toDouble());
    icon->setHeight(json["icon_height"].toDouble());
    staticIcon = new Image(this);
    staticIcon->setUrl(json["static_icon_url"].toString());
    staticIcon->setWidth(json["static_icon_width"].toDouble());
    staticIcon->setHeight(json["static_icon_height"].toDouble());

    if(json.contains("is_new") && json["is_new"].isBool())
    {
        _new = json["is_new"].toBool();
    }
    if(json.contains("days_of_premium") && json["days_of_premium"].isDouble())
    {
        daysOfPremium = json["days_of_premium"].toDouble();
    }
    if(json.contains("description") && json["description"].isString())
    {
        description = json["description"].toString();
    }
    if(json.contains("count") && json["count"].isDouble())
    {
        count = json["count"].toDouble();
    }
    if(json.contains("coin_price") && json["coin_price"].isDouble())
    {
        coinPrice = json["coin_price"].toDouble();
    }
    if(json.contains("award_sub_type") && json["award_sub_type"].isString())
    {
        awardSubType = json["award_sub_type"].toString();
    }
    if(json.contains("award_type") && json["award_type"].isString())
    {
        awardType = json["award_type"].toString();
    }
    if(json.contains("resized_icons") && json["resized_icons"].isArray())
    {
        for(const auto& ri : json["resized_icons"].toArray())
        {
            resizedIcons.append(new Image(ri.toObject(),this));
        }
    }
    if(json.contains("resized_static_icons") && json["resized_static_icons"].isArray())
    {
        for(const auto& ri : json["resized_static_icons"].toArray())
        {
            resizedStaticIcons.append(new Image(ri.toObject(),this));
        }
    }
    const auto images_less = [](const Image* i1,const Image* i2){
        return std::forward_as_tuple(i1->getWidth(), i1->getHeight()) < std::forward_as_tuple(i2->getWidth(), i2->getHeight());
    };
    std::sort(resizedIcons.begin(),resizedIcons.end(),images_less);
    std::sort(resizedStaticIcons.begin(),resizedStaticIcons.end(),images_less);

}

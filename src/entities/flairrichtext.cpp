#include "flairrichtext.h"
#include <QJsonObject>

FlairRichText::FlairRichText(QObject *parent)
    : QObject{parent}
{

}
FlairRichText::FlairRichText(const QJsonObject& json, QObject *parent)
    : QObject{parent}
{
    if(json.contains("a") && json["a"].isString())
    {
        a = json["a"].toString();
    }
    if(json.contains("e") && json["e"].isString())
    {
        e = json["e"].toString();
    }
    if(json.contains("t") && json["t"].isString())
    {
        t = json["t"].toString();
    }
    if(json.contains("u") && json["u"].isString())
    {
        u = json["u"].toString();
    }

}

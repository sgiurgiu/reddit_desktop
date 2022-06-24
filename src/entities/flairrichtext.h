#ifndef FLAIRRICHTEXT_H
#define FLAIRRICHTEXT_H

#include <QObject>
#include <QString>
#include <QList>

class QJsonObject;
class FlairRichText : public QObject
{
    Q_OBJECT
public:
    explicit FlairRichText(QObject *parent = nullptr);
    explicit FlairRichText(const QJsonObject& json, QObject *parent = nullptr);
    QString getE() const
    {
        return e;
    }
    QString getT() const
    {
        return t;
    }
    QString getA() const
    {
        return a;
    }
    QString getU() const
    {
        return u;
    }
private:
    QString e; // this looks to be the type
    QString t; //this looks to be the text
    QString a; // looks to be the emoji id
    QString u; //url for the emoji
};

#endif // FLAIRRICHTEXT_H

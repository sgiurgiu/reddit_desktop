#ifndef IMAGE_H
#define IMAGE_H

#include <QObject>
#include <QString>
#include <QList>

class QJsonObject;

class Image : public QObject
{
    Q_OBJECT
public:
    explicit Image(QObject *parent = nullptr);
    explicit Image(const QJsonObject& json, QObject *parent = nullptr);
    void setUrl(const QString&);
    void setWidth(int);
    void setHeight(int);
    QString getUrl() const
    {
        return url;
    }
    int getWidth() const
    {
        return width;
    }
    int getHeight() const
    {
        return height;
    }
private:
    QString url;
    int width = 0;
    int height = 0;
};

class ImageVariant : public QObject
{
    Q_OBJECT
public:
    explicit ImageVariant(QObject *parent = nullptr);
    explicit ImageVariant(const QString& kind, const QJsonObject& json, QObject *parent = nullptr);
    QString getKind() const
    {
        return kind;
    }
    Image* getSource() const
    {
        return source;
    }
    QList<Image*> getResolutions() const
    {
        return resolutions;
    }
signals:
private:
    QString kind;
    Image* source;
    QList<Image*> resolutions;
};

class ImagePreview : public QObject
{
    Q_OBJECT
public:
    explicit ImagePreview(QObject *parent = nullptr);
    explicit ImagePreview(const QJsonObject& json, QObject *parent = nullptr);
    Image* getSource() const
    {
        return source;
    }
    QList<Image*> getResolutions() const
    {
        return resolutions;
    }
    QList<ImageVariant*> getVariants() const
    {
        return variants;
    }
signals:
private:
    Image* source;
    QList<Image*> resolutions;
    QList<ImageVariant*> variants;
};


#endif // IMAGE_H

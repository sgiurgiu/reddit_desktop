#ifndef POSTSPROVIDER_H
#define POSTSPROVIDER_H

#include <QObject>
#include <QNetworkReply>
#include <QList>
class QNetworkAccessManager;
class AccessToken;
class Post;

class PostsProvider : public QObject
{
    Q_OBJECT
public:
    explicit PostsProvider(QObject *parent = nullptr);
    Q_INVOKABLE void loadPosts(const QString& path, const AccessToken* const token);

signals:
    void postsLoaded(QList<Post*> postsList);
    void failure(QString error);
private slots:
    void onNetworkAccessFinished();
    void onNetworkAccessError(QNetworkReply::NetworkError);
private:
    QNetworkAccessManager* networkAccessManager;
};

#endif // POSTSPROVIDER_H

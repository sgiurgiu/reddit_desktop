#ifndef SUBSCRIBEDSUBREDDITSPROVIDER_H
#define SUBSCRIBEDSUBREDDITSPROVIDER_H

#include <QObject>
#include <QNetworkReply>
#include <QList>

class QNetworkAccessManager;
class AccessToken;
class Subreddit;
class Multireddit;

class SubscribedSubredditsProvider : public QObject
{
    Q_OBJECT
public:
    explicit SubscribedSubredditsProvider(QObject *parent = nullptr);
    Q_INVOKABLE void loadSubscribedSubreddits(const AccessToken* const token);
signals:
    void subredditsLoaded(QList<Subreddit*> subreddits);
    void multisLoaded(QList<Multireddit*> multis);
    void failure(QString error);
private slots:
    void onSubredditsNetworkAccessFinished();
    void onMultisNetworkAccessFinished();
    void onNetworkAccessError(QNetworkReply::NetworkError);
private:
    QNetworkAccessManager* networkAccessManager;

};

#endif // SUBSCRIBEDSUBREDDITSPROVIDER_H

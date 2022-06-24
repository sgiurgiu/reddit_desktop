#ifndef POSTSLISTMODEL_H
#define POSTSLISTMODEL_H

#include <QAbstractListModel>
#include <QList>

class PostsProvider;
class AccessToken;
class Post;

class PostsListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum PostsRoles {
        PostRole = Qt::UserRole +1,
    };
    explicit PostsListModel(QObject *parent = nullptr);
    ~PostsListModel();
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const override;
    virtual QModelIndex index(int row, int column,
                     const QModelIndex &parent = QModelIndex()) const override;
    QHash<int,QByteArray> roleNames() const override;
    Q_INVOKABLE void loadPosts(const QString& path,AccessToken* token);
private slots:
    void postsLoaded(QList<Post*> postsList);
    void postsFailed(QString error);
private:
    PostsProvider* postsProvider;
    QList<Post*> postsList;
};

#endif // POSTSLISTMODEL_H

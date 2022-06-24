#include "postslistmodel.h"
#include "postsprovider.h"
#include "entities/post.h"
#include "accesstokenprovider.h"

PostsListModel::PostsListModel(QObject *parent):
    QAbstractListModel{parent}, postsProvider(new PostsProvider(this))
{
    connect(postsProvider,&PostsProvider::postsLoaded,this,&PostsListModel::postsLoaded);
    connect(postsProvider,&PostsProvider::failure,this,&PostsListModel::postsFailed);
}
PostsListModel::~PostsListModel()
{
    qDeleteAll(postsList);
}
int PostsListModel::rowCount(const QModelIndex &parent) const
{
    return postsList.count();
}
QVariant PostsListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    Post *post = static_cast<Post*>(index.internalPointer());
    if (role == Qt::DisplayRole)
    {
        return post->getTitle();
    }
    if (role == PostsRoles::PostRole)
    {
        return QVariant::fromValue(post);
    }

    return QVariant();
}
Qt::ItemFlags PostsListModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return QAbstractListModel::flags(index);
}
QModelIndex PostsListModel::index(int row, int column,
                 const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
            return QModelIndex();

    if(row < 0 || row > postsList.count())
        return QModelIndex();

    return createIndex(row,column,postsList.at(row));
}
QHash<int,QByteArray> PostsListModel::roleNames() const
{
    return {
        { Qt::DisplayRole, "display" },
        { PostsRoles::PostRole , "post" },
    };
}

void PostsListModel::loadPosts(const QString& path, AccessToken* token)
{
    postsProvider->loadPosts(path,token);
}
void PostsListModel::postsLoaded(QList<Post*> postsList)
{
    beginResetModel();
    this->postsList = std::move(postsList);
    endResetModel();
}
void PostsListModel::postsFailed(QString error)
{

}

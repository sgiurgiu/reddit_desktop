#ifndef SUBREDDITSLISTMODEL_H
#define SUBREDDITSLISTMODEL_H

#include <QAbstractItemModel>
#include <QList>
#include <QVariant>
class AccessToken;
class SubredditBase;
class SubscribedSubredditsProvider;
class Multireddit;
class Subreddit;
class SubredditsListItem
{
public:
    explicit SubredditsListItem(const QString& name, SubredditsListItem* parent = nullptr);
    explicit SubredditsListItem(const SubredditBase* sr, SubredditsListItem* parent);

    ~SubredditsListItem();
    void appendChild(SubredditsListItem *child);
    SubredditsListItem *child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    SubredditsListItem *getParentItem() const;
    bool isSelected() const;
    void setSelected(bool flag);
    bool isMulti() const;
    QString getUrl() const;
    QString getName() const;
private:
    QList<SubredditsListItem *> childItems;
    QVariant itemData;
    SubredditsListItem *parentItem = nullptr;
    bool selected = false;
    bool multi = false;
    QString url;
    QString name;
};

class SubredditsListModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    enum SubredditsRoles {
        SelectedRole = Qt::UserRole +1,
        UrlRole,
        MultiRole,
        NameRole
    };
    explicit SubredditsListModel(QObject* parent = nullptr);
    ~SubredditsListModel();
    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                       int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column,
                     const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int,QByteArray> roleNames() const override;
    Q_INVOKABLE void loadSubscribedSubreddits(AccessToken* token);
private slots:
    void multisLoaded(QList<Multireddit*> multis);
    void subredditsLoaded(QList<Subreddit*> subreddits);
private:
    SubredditsListItem* rootItem;
    SubredditsListItem* multisItem;
    SubredditsListItem* subredditsItem;
    SubscribedSubredditsProvider* dataProvider;
};

#endif // SUBREDDITSLISTMODEL_H

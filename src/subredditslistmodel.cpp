#include "subredditslistmodel.h"
#include "entities/subreddit.h"
#include "subscribedsubredditsprovider.h"

SubredditsListModel::SubredditsListModel(QObject* parent):QAbstractItemModel{parent},
    dataProvider(new SubscribedSubredditsProvider(this))
{
    rootItem = new SubredditsListItem("Root");
    multisItem = new SubredditsListItem("Multis",rootItem);
    subredditsItem = new SubredditsListItem("Subscribed", rootItem);
    rootItem->appendChild(multisItem);
    rootItem->appendChild(subredditsItem);
    connect(dataProvider,&SubscribedSubredditsProvider::multisLoaded,this,&SubredditsListModel::multisLoaded);
    connect(dataProvider,&SubscribedSubredditsProvider::subredditsLoaded,this,&SubredditsListModel::subredditsLoaded);
}
SubredditsListModel::~SubredditsListModel()
{
    delete rootItem;
}

void SubredditsListModel::loadSubscribedSubreddits(AccessToken* token)
{
    dataProvider->loadSubscribedSubreddits(token);
}
void SubredditsListModel::multisLoaded(QList<Multireddit*> multis)
{
    beginResetModel();
    for(auto m:multis)
    {
        multisItem->appendChild(new SubredditsListItem(m,multisItem));
    }
    endResetModel();

}
void SubredditsListModel::subredditsLoaded(QList<Subreddit*> subreddits)
{
    beginResetModel();
    for(auto s:subreddits)
    {
        subredditsItem->appendChild(new SubredditsListItem(s,subredditsItem));
    }

    endResetModel();
}

QVariant SubredditsListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    SubredditsListItem *item = static_cast<SubredditsListItem*>(index.internalPointer());
    if (role == Qt::DisplayRole)
    {
        return item->data(index.column());
    }
    if(role == SubredditsRoles::SelectedRole)
    {
        return QVariant(item->isSelected());
    }
    if(role == SubredditsRoles::UrlRole)
    {
        return QVariant(item->getUrl());
    }
    if(role == SubredditsRoles::MultiRole)
    {
        return QVariant(item->isMulti());
    }
    if(role == SubredditsRoles::NameRole)
    {
        return QVariant(item->getName());
    }
    return QVariant();
}
Qt::ItemFlags SubredditsListModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return QAbstractItemModel::flags(index);
}
QVariant SubredditsListModel::headerData(int section, Qt::Orientation orientation,
                   int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section);

    return QVariant();
}
QModelIndex SubredditsListModel::index(int row, int column,
                 const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
            return QModelIndex();

    SubredditsListItem *parentSubredditListItem;

    if (!parent.isValid())
        parentSubredditListItem = rootItem;
    else
        parentSubredditListItem = static_cast<SubredditsListItem*>(parent.internalPointer());

    SubredditsListItem *childItem = parentSubredditListItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    return QModelIndex();
}
QModelIndex SubredditsListModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();
    SubredditsListItem *childItem = static_cast<SubredditsListItem*>(index.internalPointer());
    SubredditsListItem *parent = childItem->getParentItem();

    if (!parent  || parent == rootItem)
        return QModelIndex();

    return createIndex(parent->row(), 0, parent);
}
int SubredditsListModel::rowCount(const QModelIndex &parent) const
{
    SubredditsListItem *parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<SubredditsListItem*>(parent.internalPointer());

    return parentItem->childCount();
}
int SubredditsListModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return static_cast<SubredditsListItem*>(parent.internalPointer())->columnCount();
    return rootItem->columnCount();
}
QHash<int,QByteArray> SubredditsListModel::roleNames() const
{
    return {
        { Qt::DisplayRole, "display" },
        { SubredditsRoles::SelectedRole , "selected" },
        { SubredditsRoles::UrlRole , "url" },
        { SubredditsRoles::MultiRole , "multi" },
        { SubredditsRoles::NameRole , "name" },
    };
}
SubredditsListItem::~SubredditsListItem()
{
    qDeleteAll(childItems);
}

SubredditsListItem::SubredditsListItem(const QString& name, SubredditsListItem* parent):
    itemData{name},parentItem{parent}
{
}
SubredditsListItem::SubredditsListItem(const SubredditBase* sr, SubredditsListItem* parent):
    itemData{QVariant::fromValue(sr)},parentItem{parent}
{
    multi = sr->isMulti();
    url = sr->getTargetUrl();
    name = sr->getDisplayName();
}

void SubredditsListItem::appendChild(SubredditsListItem *child)
{
    childItems.append(child);
    child->parentItem = this;
}
SubredditsListItem *SubredditsListItem::child(int row)
{
    if(row < 0 || row >= childItems.size())
        return nullptr;
    return childItems.at(row);
}
int SubredditsListItem::childCount() const
{
    return childItems.size();
}
int SubredditsListItem::columnCount() const
{
    return 1;
}
QVariant SubredditsListItem::data(int column) const
{
    if(column != 0) return QVariant();

    if(itemData.userType() == QMetaType::QString)
    {
        return itemData;
    }
    else if(itemData.userType() == qMetaTypeId<SubredditBase*>())
    {
        return QVariant(itemData.value<SubredditBase*>()->getDisplayName());
    }
    return QVariant();
}
int SubredditsListItem::row() const
{
    if (parentItem)
        return parentItem->childItems.indexOf(const_cast<SubredditsListItem*>(this));

    return 0;
}
SubredditsListItem *SubredditsListItem::getParentItem() const
{
    return parentItem;
}

bool SubredditsListItem::isSelected() const
{
    return selected;
}
void SubredditsListItem::setSelected(bool flag)
{
    selected = flag;
}
bool SubredditsListItem::isMulti() const
{
    return multi;
}
QString SubredditsListItem::getUrl() const
{
    return url;
}
QString SubredditsListItem::getName() const
{
    return name;
}

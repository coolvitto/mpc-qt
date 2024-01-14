#ifndef QDRAWNPLAYLIST_H
#define QDRAWNPLAYLIST_H

#include <QListWidget>
#include <QUuid>
#include <functional>
#include "playlist.h"

class DisplayParser;
class QThread;
class PlaylistSearcher;

class PlayPainter : public QAbstractItemDelegate {
    Q_OBJECT
public:
    PlayPainter(QObject *parent = nullptr);
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const;
};


class PlayItem : public QListWidgetItem {
public:
    PlayItem(const QUuid &uuid = QUuid(), const QUuid &playlistUuid = QUuid(), QListWidget *parent = nullptr);
    ~PlayItem();

    QUuid playlistUuid();
    QUuid uuid();

private:
    QUuid playlistUuid_;
    QUuid uuid_;
};


// DrawnPlaylist abuses a ListWidget's items to store the uuid of playlist
// items, which is decoded and looked up during drawing.  The alternative is a
// subclass of QAbstractItemModel with about 1.5x the code.
class DrawnPlaylist : public QListWidget {
    Q_OBJECT
public:
    DrawnPlaylist(QSharedPointer<PlaylistCollection> collection, QWidget *parent = nullptr);
    ~DrawnPlaylist();
    void setCollection(QSharedPointer<PlaylistCollection> collection);
    virtual QSharedPointer<Playlist> playlist() const;
    QUuid uuid() const;
    void setUuid(const QUuid &uuid);
    QUuid currentItemUuid() const;
    QList<QUuid> currentItemUuids() const;
    bool thumbsShown();

    void traverseSelected(std::function<void(QUuid)> callback);
    void setCurrentItem(QUuid itemUuid);
    void scrollToItem(QUuid itemUuid);

    virtual void addItem(QUuid uuid);
    void addItems(const QList<QUuid> &items);
    void addItemsAfter(QUuid item, const QList<QUuid> &items);
    void removeItem(QUuid uuid);
    void removeItems(const QList<int> &indicies);
    void removeAll();
    template<class T>
    void sort(std::function<T(QSharedPointer<Item>)> converter,
              std::function<bool(const T &a, const T &b)> lessThan);

    QPair<QUuid,QUuid> importUrl(QUrl url);
    void currentToQueue();

    QUuid nowPlayingItem();
    void setNowPlayingItem(QUuid uuid);

    QVariantMap toVMap() const;
    void fromVMap(const QVariantMap &qvm);

    void setDisplayParser(DisplayParser *parser);
    DisplayParser *displayParser();

    void setFilter(QString needles);

protected:
    bool event(QEvent *e);

private:
    QSharedPointer<PlaylistCollection> collection_;
    QUuid uuid_;
    QHash <QUuid, PlayItem*> itemsByUuid;
    QUuid lastSelectedItem;
    QUuid nowPlayingItem_;
    DisplayParser *displayParser_ = nullptr;
    QThread *worker = nullptr;
    PlaylistSearcher *searcher;
    QString currentFilterText;
    QStringList currentFilterList;
    bool thumbsShown_ = false;

signals:
    // for lack of a better term that doesn't conflict with what we already
    // have, when an item is made hot by double clicking.
    void itemDesired(QUuid playlistUuid, QUuid itemUuid);
    void searcher_filterPlaylist(QSharedPointer<Playlist>, QString text);
    void menuOpenItem(QUuid playlistUuid, QUuid itemUuid);

    void contextMenuRequested(QPoint p, QUuid playlistUuid, QUuid itemUuid);

public slots:
    void setThumbnailsShown(bool visible);

private slots:
    void repopulateItems();

    void model_rowsMoved(const QModelIndex & parent, int start, int end,
                         const QModelIndex & destination, int row);
    void self_currentItemChanged(QListWidgetItem *current,
                                 QListWidgetItem *previous);
    void self_itemDoubleClicked(QListWidgetItem *item);
    void self_customContextMenuRequested(const QPoint &p);
};

template<class T>
void DrawnPlaylist::sort(
        std::function<T(QSharedPointer<Item>)> converter,
        std::function<bool(const T &a, const T &b)> lessThan) {
    QMap<QUuid,T> playlistMap;
    QList<QSharedPointer<Item>> items;
    auto pl = playlist();
    int index = 0;
    pl->iterateItems([&](QSharedPointer<Item> i) {
        playlistMap.insert(i->uuid(), converter(i));
        items.append(i);
        i->setOriginalPosition(index++);
    });
    std::sort(items.begin(), items.end(), [&](const QSharedPointer<Item> &a, const QSharedPointer<Item> &b) {
        return lessThan(playlistMap.value(a->uuid()), playlistMap.value(b->uuid()));
    });
    pl->takeItemsRaw(items);
    pl->addItems(QUuid(), items);
    repopulateItems();
}


class DrawnQueue : public DrawnPlaylist {
    Q_OBJECT
public:
    DrawnQueue();
    virtual QSharedPointer<Playlist> playlist() const;
    void addItem(QUuid uuid);
};

class PlaylistSelectionPrivate;
class PlaylistSelection {
public:
    PlaylistSelection();
    PlaylistSelection(PlaylistSelection &other);
    ~PlaylistSelection();
    void fromItem(QUuid playlistUuid, QUuid itemUuid);
    void fromQueue(DrawnPlaylist *list);
    void fromSelected(DrawnPlaylist *list);

    void appendToPlaylist(DrawnPlaylist *list);
    void appendAndQuickQueue(DrawnPlaylist *list);

private:
    PlaylistSelectionPrivate *d = nullptr;
};
#endif // QDRAWNPLAYLIST_H

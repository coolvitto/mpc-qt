#include "playlistwindow.h"
#include "ui_playlistwindow.h"
#include "qdrawnplaylist.h"
#include "playlist.h"
#include <QDragEnterEvent>
#include <QMimeData>
#include <QInputDialog>
#include <QFileDialog>
#include <QMenu>
#include <QThread>

PlaylistWindow::PlaylistWindow(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::PlaylistWindow),
    currentPlaylist(),
    showSearch(false)
{
    // When (un)docking windows, some widgets may get transformed into native
    // widgets, causing painting glitches.  Tell Qt that we prefer non-native.
    setAttribute(Qt::WA_DontCreateNativeAncestors);

    ui->setupUi(this);
    addNewTab(QUuid(), tr("Quick Playlist"));
    ui->searchField->setVisible(false);
    ui->searchField->installEventFilter(this);

    connect(this, &PlaylistWindow::visibilityChanged,
            this, &PlaylistWindow::self_visibilityChanged);
}

PlaylistWindow::~PlaylistWindow()
{
    delete ui;
}

void PlaylistWindow::setCurrentPlaylist(QUuid what)
{
    if (widgets.contains(what)) {
        ui->tabWidget->setCurrentWidget(widgets[what]);
        currentPlaylist = what;
    }
    updateCurrentPlaylist();
}

void PlaylistWindow::clearPlaylist(QUuid what)
{
    if (widgets.contains(what))
        widgets[what]->removeAll();
}

QPair<QUuid, QUuid> PlaylistWindow::addToCurrentPlaylist(QList<QUrl> what)
{
    QPair<QUuid, QUuid> info;
    auto qdp = reinterpret_cast<QDrawnPlaylist *>(ui->tabWidget->currentWidget());
    for (QUrl url : what) {
        QPair<QUuid,QUuid> itemInfo = qdp->importUrl(url);
        if (info.second.isNull())
            info = itemInfo;
    }
    return info;
}

QPair<QUuid, QUuid> PlaylistWindow::urlToQuickPlaylist(QUrl what)
{
    auto pl = PlaylistCollection::getSingleton()->playlistOf(QUuid());
    pl->clear();
    widgets[QUuid()]->clear();
    ui->tabWidget->setCurrentWidget(widgets[QUuid()]);
    return addToCurrentPlaylist(QList<QUrl>() << what);
}

bool PlaylistWindow::isCurrentPlaylistEmpty()
{
    auto pl = PlaylistCollection::getSingleton()->playlistOf(currentPlaylist);
    return pl ? pl->isEmpty() : true;
}

QUuid PlaylistWindow::getItemAfter(QUuid list, QUuid item)
{
    auto pl = PlaylistCollection::getSingleton()->playlistOf(list);
    if (!pl)
        return QUuid();
    QUuid uuid = pl->queueTakeFirst();
    if (!uuid.isNull())
        return uuid;
    QSharedPointer<Item> after = pl->itemAfter(item);
    if (!after)
        return QUuid();
    return after->uuid();
}

QUuid PlaylistWindow::getItemBefore(QUuid list, QUuid item)
{
    auto pl = PlaylistCollection::getSingleton()->playlistOf(list);
    if (!pl)
        return QUuid();
    QSharedPointer<Item> before = pl->itemBefore(item);
    if (!before)
        return QUuid();
    return before->uuid();
}

QUrl PlaylistWindow::getUrlOf(QUuid list, QUuid item)
{
    auto pl = PlaylistCollection::getSingleton()->playlistOf(list);
    if (!pl)
        return QUrl();
    auto i = pl->itemOf(item);
    if (!i)
        return QUrl();
    return i->url();
}

void PlaylistWindow::setMetadata(QUuid list, QUuid item, const QVariantMap &map)
{
    auto pl = PlaylistCollection::getSingleton()->playlistOf(list);
    if (!pl)
        return;
    auto i = pl->itemOf(item);
    if (!i)
        return;
    i->setMetadata(map);

    auto qdp = reinterpret_cast<QDrawnPlaylist *>(ui->tabWidget->currentWidget());
    if (qdp->uuid() == list)
        qdp->viewport()->update();

}

QVariantList PlaylistWindow::tabsToVList() const
{
    QVariantList qvl;
    for (int i = 0; i < ui->tabWidget->count(); i++) {
        auto widget = reinterpret_cast<QDrawnPlaylist *>(ui->tabWidget->widget(i));
        qvl.append(widget->toVMap());
    }
    return qvl;
}

void PlaylistWindow::tabsFromVList(const QVariantList &qvl)
{
    ui->tabWidget->clear();
    widgets.clear();
    for (const QVariant &v : qvl) {
        auto qdp = new QDrawnPlaylist();
        qdp->setDisplayParser(&displayParser);
        qdp->fromVMap(v.toMap());
        connect(qdp, &QDrawnPlaylist::itemDesired, this, &PlaylistWindow::itemDesired);
        auto pl = PlaylistCollection::getSingleton()->playlistOf(qdp->uuid());
        ui->tabWidget->addTab(qdp, pl->title());
        widgets.insert(pl->uuid(), qdp);
    }
    if (widgets.count() < 1)
        addNewTab(QUuid(), tr("Quick Playlist"));
}

void PlaylistWindow::selectNext()
{
    auto qdp = reinterpret_cast<QDrawnPlaylist *>(ui->tabWidget->currentWidget());
    int index = qdp->currentRow();
    if (index < qdp->count())
        qdp->setCurrentRow(index + 1);
}

void PlaylistWindow::selectPrevious()
{
    auto qdp = reinterpret_cast<QDrawnPlaylist *>(ui->tabWidget->currentWidget());
    int index = qdp->currentRow();
    if (index > 0)
        qdp->setCurrentRow(index - 1);
}

void PlaylistWindow::quickQueue()
{
    auto qdp = reinterpret_cast<QDrawnPlaylist *>(ui->tabWidget->currentWidget());
    auto pl = PlaylistCollection::getSingleton()->playlistOf(qdp->uuid());
    auto itemUuid = qdp->currentItemUuid();
    if (itemUuid.isNull())
        return;
    pl->queueToggle(itemUuid);
    qdp->viewport()->update();
}

void PlaylistWindow::revealSearch()
{
    showSearch = true;
    activateWindow();
    ui->searchField->setVisible(true);
    ui->searchField->setFocus();
}

void PlaylistWindow::finishSearch()
{
    showSearch = false;
    if (!ui->searchField->isVisible())
        return;

    if (!ui->searchField->text().isEmpty()) {
        ui->searchField->setText(QString());
        setPlaylistFilters(QString());
    }

    ui->searchField->setVisible(false);
}

bool PlaylistWindow::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj);
    if (obj == ui->searchField && event->type() == QEvent::KeyPress) {
        auto keyEvent = reinterpret_cast<QKeyEvent*>(event);
        if (!keyEvent->modifiers() &&
                (keyEvent->key() == Qt::Key_Up ||
                 keyEvent->key() == Qt::Key_Down)) {
            if (keyEvent->key() == Qt::Key_Up)
                selectPrevious();
            else
                selectNext();
            return true;
        }
    }
    return QDockWidget::eventFilter(obj, event);
}

void PlaylistWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
        event->accept();
}

void PlaylistWindow::dropEvent(QDropEvent *event)
{
    if (!event->mimeData()->hasUrls())
        return;
    addToCurrentPlaylist(event->mimeData()->urls());
}

void PlaylistWindow::wheelEvent(QWheelEvent *event)
{
    // Don't pass scroll events up the chain.  They are used for e.g. tab
    // switching when over the tab bar and also scrolling the playlists.
    event->accept();
}

void PlaylistWindow::updateCurrentPlaylist()
{
    auto qdp = reinterpret_cast<QDrawnPlaylist *>(ui->tabWidget->currentWidget());
    if (!qdp)
        return;
    currentPlaylist = qdp->uuid();
}

void PlaylistWindow::setPlaylistFilters(QString filterText)
{
    for (auto widget : widgets) {
        widget->setFilter(filterText);
    }
}

void PlaylistWindow::addNewTab(QUuid playlist, QString title)
{
    auto qdp = new QDrawnPlaylist();
    qdp->setDisplayParser(&displayParser);
    qdp->setUuid(playlist);
    connect(qdp, &QDrawnPlaylist::itemDesired, this, &PlaylistWindow::itemDesired);
    widgets.insert(playlist, qdp);
    ui->tabWidget->addTab(qdp, title);
}

void PlaylistWindow::changePlaylistSelection( QUrl itemUrl, QUuid playlistUuid, QUuid itemUuid)
{
    (void)itemUrl;
    if (!widgets.contains(playlistUuid))
        return;
    auto qdp = widgets[playlistUuid];
    auto pl = PlaylistCollection::getSingleton()->playlistOf(playlistUuid);
    if (!itemUuid.isNull() && pl->queueFirst() == itemUuid)
        pl->queueTakeFirst();
    qdp->scrollToItem(itemUuid);
    qdp->setNowPlayingItem(itemUuid);
}

void PlaylistWindow::addSimplePlaylist(QStringList data)
{
    auto pl = PlaylistCollection::getSingleton()->newPlaylist(tr("New Playlist"));
    pl->fromStringList(data);
    addNewTab(pl->uuid(), pl->title());
}

void PlaylistWindow::setDisplayFormatSpecifier(QString fmt)
{
    displayParser.takeFormatString(fmt);
    ui->tabWidget->currentWidget()->update();
}

void PlaylistWindow::playCurrentItem()
{
    auto qdp = reinterpret_cast<QDrawnPlaylist *>(ui->tabWidget->currentWidget());
    auto pl = PlaylistCollection::getSingleton()->playlistOf(qdp->uuid());
    auto itemUuid = qdp->currentItemUuid();
    if (itemUuid.isNull())
        return;
    emit itemDesired(pl->uuid(), itemUuid);
}

void PlaylistWindow::self_relativeSeekRequested(bool forwards, bool small)
{
    emit relativeSeekRequested(forwards, small);
}

void PlaylistWindow::self_visibilityChanged()
{
    // When the window was (re)created/destroyed for whatever reason by
    // the toolkit/wm/etc, reveal the search widget if it was active last.
    if (showSearch)
        revealSearch();
    else
        finishSearch();
}

void PlaylistWindow::on_newTab_clicked()
{
    auto pl = PlaylistCollection::getSingleton()->newPlaylist(tr("New Playlist"));
    addNewTab(pl->uuid(), pl->title());
}

void PlaylistWindow::on_closeTab_clicked()
{
    int index = ui->tabWidget->currentIndex();
    on_tabWidget_tabCloseRequested(index);
    updateCurrentPlaylist();
}

void PlaylistWindow::on_tabWidget_tabCloseRequested(int index)
{
    int current = ui->tabWidget->currentIndex();
    auto qdp = reinterpret_cast<QDrawnPlaylist *>(ui->tabWidget->widget(index));
    if (!qdp)
        return;
    if (qdp->uuid().isNull()) {
        qdp->removeAll();
    } else {
        PlaylistCollection::getSingleton()->removePlaylist(qdp->uuid());
        widgets.remove(qdp->uuid());
        ui->tabWidget->removeTab(index);
    }
    if (current == index)
        updateCurrentPlaylist();
}

void PlaylistWindow::on_duplicateTab_clicked()
{
    auto origin = reinterpret_cast<QDrawnPlaylist *>(ui->tabWidget->currentWidget());
    auto remote = PlaylistCollection::getSingleton()->clonePlaylist(origin->uuid());
    addNewTab(remote->uuid(), remote->title());
}

void PlaylistWindow::on_tabWidget_tabBarDoubleClicked(int index)
{
    auto widget = reinterpret_cast<QDrawnPlaylist *>(ui->tabWidget->widget(index));
    QUuid tabUuid = widget->uuid();
    if (tabUuid.isNull())
        return;
    QInputDialog *qid = new QInputDialog(this);
    qid->setAttribute(Qt::WA_DeleteOnClose);
    qid->setWindowModality(Qt::ApplicationModal);
    qid->setWindowTitle(tr("Enter playlist name"));
    qid->setTextValue(ui->tabWidget->tabText(index));
    connect(qid, &QInputDialog::accepted, [=]() {
        int tabIndex = ui->tabWidget->indexOf(widget);
        if (tabIndex < 0)
            return;
        auto pl = PlaylistCollection::getSingleton()->playlistOf(tabUuid);
        if (!pl)
            return;
        pl->setTitle(qid->textValue());
        ui->tabWidget->setTabText(tabIndex, qid->textValue());
    });
    qid->show();
}

void PlaylistWindow::on_importList_clicked()
{
    QString file;
    file = QFileDialog::getOpenFileName(this, tr("Import File"), QString(),
                                        tr("Playlist files (*.m3u *.m3u8)"));
    if (!file.isEmpty())
        emit importPlaylist(file);
}

void PlaylistWindow::on_exportList_clicked()
{
    auto uuid = reinterpret_cast<QDrawnPlaylist*>(ui->tabWidget->currentWidget())->uuid();

    QString file;
    file = QFileDialog::getSaveFileName(this, tr("Export File"), QString(),
                                        tr("Playlist files (*.m3u *.m3u8)"));
    auto pl = PlaylistCollection::getSingleton()->playlistOf(uuid);
    if (!file.isEmpty() && pl)
        emit exportPlaylist(file, pl->toStringList());
}

void PlaylistWindow::on_tabWidget_customContextMenuRequested(const QPoint &pos)
{
    QMenu *m = new QMenu(this);
    m->addAction(tr("&New Playlist"), this, SLOT(on_newTab_clicked()));
    m->addAction(tr("&Remove Playlist"), this, SLOT(on_closeTab_clicked()));
    m->addAction(tr("&Duplicate Playlist"), this, SLOT(on_duplicateTab_clicked()));
    m->addAction(tr("&Import Playlist"), this, SLOT(on_importList_clicked()));
    m->addAction(tr("&Export Playlist"), this, SLOT(on_exportList_clicked()));
    m->exec(ui->tabWidget->mapToGlobal(pos));
}

void PlaylistWindow::on_searchField_textEdited(const QString &arg1)
{
    setPlaylistFilters(arg1);
}

void PlaylistWindow::on_tabWidget_currentChanged(int index)
{
    updateCurrentPlaylist();
}


#ifndef MAIN_H
#define MAIN_H
#include <QHash>
#include <QMetaMethod>
#include "helpers.h"
#include "mainwindow.h"
#include "manager.h"
#include "storage.h"
#include "settingswindow.h"

// a simple class to control program exection and own application objects
class Flow : public QObject {
    Q_OBJECT
public:
    explicit Flow(QObject *owner = 0);
    ~Flow();

    int run();
    bool hasPrevious();

signals:
    void recentFilesChanged(QList<TrackInfo> urls);

private:
    void setupIpcCommands();
    QByteArray makePayload() const;
    void socketReturn(QLocalSocket *socket, bool wasParsed, QVariant value = QVariant());
    QString pictureTemplate(Helpers::DisabledTrack tracks, Helpers::Subtitles subs) const;
    QVariantList recentToVList() const;
    void recentFromVList(const QVariantList &list);
    QVariantMap saveWindows();
    void restoreWindows(const QVariantMap &map);

private slots:
    void mainwindow_applicationShouldQuit();
    void mainwindow_recentOpened(const TrackInfo &track);
    void mainwindow_recentClear();
    void mainwindow_takeImage();
    void mainwindow_takeImageAutomatically();
    void mainwindow_optionsOpenRequested();
    void manager_nowPlayingChanged(QUrl url, QUuid listUuid, QUuid itemUuid);
    void server_payloadRecieved(const QByteArray &payload, QLocalSocket *socket);
    void ipc_playFiles(const QVariantMap &map);
    void ipc_play(const QVariantMap &map);
    void ipc_pause();
    void ipc_unpause();
    void ipc_start();
    void ipc_stop();
    void ipc_next(const QVariantMap &map);
    void ipc_previous(const QVariantMap &map);
    void ipc_repeat();
    void ipc_togglePlayback();
    QVariant ipc_getMpvProperty(const QVariantMap &map);
    QVariant ipc_setMpvProperty(const QVariantMap &map);
    QVariant ipc_setMpvOption(const QVariantMap &map);
    QVariant ipc_doMpvCommand(const QVariantMap &map);
    void settingswindow_settingsData(const QVariantMap &settings);
    void settingswindow_rememberWindowGeometry(bool yes);
    void settingswindow_keymapData(const QVariantMap &keyMap);
    void settingswindow_screenshotDirectory(const QString &where);
    void settingswindow_encodeDirectory(const QString &where);
    void settingswindow_screenshotTemplate(const QString &fmt);
    void settingswindow_encodeTemplate(const QString &fmt);
    void settingswindow_screenshotFormat(const QString &fmt);
    void importPlaylist(QString fname);
    void exportPlaylist(QString fname, QStringList items);

private:
    JsonServer *server;
    MainWindow *mainWindow;
    PlaybackManager *playbackManager;
    SettingsWindow *settingsWindow;
    QHash<QString, QMetaMethod> ipcCommands;
    Storage storage;
    QVariantMap settings;
    QVariantMap keyMap;
    QList<TrackInfo> recentFiles;

    bool rememberWindowGeometry;
    QString screenshotDirectory;
    QString encodeDirectory;
    QString screenshotTemplate;
    QString encodeTemplate;
    QString screenshotFormat;
    bool hasPrevious_;
};

#endif // MAIN_H

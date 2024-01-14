#ifndef STORAGE_H
#define STORAGE_H

#include <QObject>

class Storage : public QObject
{
    Q_OBJECT
public:
    explicit Storage(QObject *parent = nullptr);
    static QString fetchConfigPath();
    static QString fetchThumbnailPath();

    void writeVMap(QString name, const QVariantMap &qvm);
    QVariantMap readVMap(QString name);

    void writeVList(QString name, const QVariantList &qvl);
    QVariantList readVList(QString name);

    QStringList readM3U(const QString &where);
    void writeM3U(const QString &where, QStringList items);

private:
    void writeJsonObject(QString fname, const QJsonDocument &doc);
    QJsonDocument readJsonObject(QString fname);

signals:

public slots:

private:
    static QString configPath;
    static QString thumbPath;
};

#endif // STORAGE_H

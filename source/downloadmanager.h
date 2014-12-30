#ifndef DOWNLOADMANAGER_H
#define DOWNLOADMANAGER_H

#include <QObject>
#include <QtCore/QMap>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkAccessManager>

class QIODevice;
class QNetworkReply;

class DownloadManager : public QObject
{
    Q_OBJECT
public:
    DownloadManager(QObject* parent = 0);
    void doDownload(const QUrl& url, const QString& filename);

private:
    bool saveToDisk(QNetworkReply* data);

signals:
    void progress(int);
    void error(QString);
    void downloadFinished(QString);
    void allDownloadsFinished();

public slots:
    void cancelAllDownloads();

private slots:
    void downloadFinished(QNetworkReply* reply);
    void downloadProgress(qint64 received, qint64 total);

private:
    QNetworkAccessManager         m_manager;
    QMap<QNetworkReply*, QString> m_current_downloads;
};

#endif // DOWNLOADMANAGER_H

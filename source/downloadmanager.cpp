#include "downloadmanager.h"
#include <QtCore/QIODevice>
#include <QtCore/QDebug>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtCore/QFileInfo>

DownloadManager::DownloadManager(QObject* parent)
    : QObject(parent)
{
    QObject::connect(&m_manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadFinished(QNetworkReply*)));
}

void DownloadManager::doDownload(const QUrl& url, const QString& filename)
{
    QNetworkRequest request(url);
    QNetworkReply* reply = m_manager.get(request);

    QObject::connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(downloadProgress(qint64,qint64)));
    m_current_downloads[reply] = filename;
}

bool DownloadManager::saveToDisk(QNetworkReply* data)
{
    QString& filename = m_current_downloads[data];
    QFile file(filename);

    if (!file.open(QIODevice::WriteOnly))
    {
        qDebug("Could not open %s for writing: %s\n", qPrintable(filename), qPrintable(file.errorString()));
        return false;
    }

    file.write(data->readAll());
    file.close();

    emit downloadFinished(filename);

    return true;
}

void DownloadManager::cancelAllDownloads()
{/*
    for (auto i = m_current_downloads.begin(), end = m_current_downloads.end(); i != end; ++i)
    {
        i->first->abort();
        i->first->deleteLater();
    }

    m_current_downloads.clear(); */
}

void DownloadManager::downloadFinished(QNetworkReply* reply)
{
    if (reply->error())
    {
        qDebug() << reply->errorString();
        emit error(reply->errorString());
    }
    else
    {
        if (saveToDisk(reply))
            qDebug("Download of %s succeded", reply->url().toEncoded().constData());
    }

    m_current_downloads.remove(reply);
    reply->deleteLater();

    if (m_current_downloads.empty())
        emit allDownloadsFinished();
}

void DownloadManager::downloadProgress(qint64 received, qint64 total)
{
    if (received && total > 0)
        emit progress(100.0 * received  / total);
}

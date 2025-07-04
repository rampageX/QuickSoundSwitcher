#ifndef UPDATEMANAGER_H
#define UPDATEMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QProcess>
#include <QThread>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCoreApplication>
#include <QDebug>

class UpdateManager : public QObject
{
    Q_OBJECT

public:
    explicit UpdateManager(QObject* parent = nullptr);
    void startUpdate(const QString& downloadUrl, const QString& targetPath);

signals:
    void statusChanged(const QString& status);
    void progressChanged(double progress);
    void finished();
    void error(const QString& errorText);

private slots:
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished();

private:
    QNetworkAccessManager* m_network;
    QNetworkReply* m_reply;
    QString m_targetPath;
    QString m_tempPath;

    void performBackup();
    void installUpdate();
    void launchApp();
    void cleanup();
    bool isQSSRunning();
    void waitForQSSToClose();
    bool copyDirectoryContents(const QString& source, const QString& destination);
    bool copyFileWithRetries(const QString& source, const QString& dest, const QString& fileName);
};

#endif // UPDATEMANAGER_H

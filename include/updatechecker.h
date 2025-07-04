#ifndef UPDATECHECKER_H
#define UPDATECHECKER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QSettings>

struct VersionInfo {
    QString version;
    QString downloadUrl;
    QString releaseNotes;
    bool isNewer;
};

class UpdateChecker : public QObject
{
    Q_OBJECT

public:
    explicit UpdateChecker(QObject* parent = nullptr);
    ~UpdateChecker();

    void startPeriodicCheck();
    void stopPeriodicCheck();
    void checkForUpdates();

    Q_INVOKABLE void checkForUpdatesManually(); // Add this

signals:
    void updateAvailable(const VersionInfo& info);
    void updateCheckFinished(bool hasUpdate, const VersionInfo& info);
    void updateCheckFailed(const QString& error);
    void manualCheckStarted(); // Add this
    void manualCheckCompleted(bool hasUpdate); // Add this

private slots:
    void onUpdateCheckFinished(QNetworkReply* reply);

private:
    QNetworkAccessManager* m_networkManager;
    QTimer* m_checkTimer;
    QSettings m_settings;

    bool isVersionNewer(const QString& current, const QString& available);
    QString getCurrentVersion() const;
    void parseGitHubResponse(const QByteArray& data, VersionInfo& info);
};

#endif // UPDATECHECKER_H

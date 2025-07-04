#include "updatechecker.h"
#include "version.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVersionNumber>
#include <QDebug>

UpdateChecker::UpdateChecker(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_checkTimer(new QTimer(this))
    , m_settings("Odizinne", "QuickSoundSwitcher")
{
    // Check every 4 hours (4 * 60 * 60 * 1000 ms)
    m_checkTimer->setInterval(4 * 60 * 60 * 1000);
    m_checkTimer->setSingleShot(false);
    
    connect(m_checkTimer, &QTimer::timeout, this, &UpdateChecker::checkForUpdates);
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &UpdateChecker::onUpdateCheckFinished);
}

UpdateChecker::~UpdateChecker()
{
    stopPeriodicCheck();
}

void UpdateChecker::startPeriodicCheck()
{
    if (m_settings.value("autoUpdateCheck", true).toBool()) {
        m_checkTimer->start();
        // Check 30 seconds after startup
    }
}

void UpdateChecker::stopPeriodicCheck()
{
    m_checkTimer->stop();
}

void UpdateChecker::checkForUpdates()
{
    qDebug() << "Checking for updates...";
    
    QNetworkRequest request;
    request.setUrl(QUrl("https://api.github.com/repos/Odizinne/QuickSoundSwitcher/releases/latest"));
    request.setRawHeader("User-Agent", "QuickSoundSwitcher-UpdateChecker");
    request.setRawHeader("Accept", "application/vnd.github.v3+json");
    
    m_networkManager->get(request);
}

void UpdateChecker::parseGitHubResponse(const QByteArray& data, VersionInfo& info)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        throw std::runtime_error(error.errorString().toStdString());
    }
    
    QJsonObject obj = doc.object();
    info.version = obj["tag_name"].toString();
    info.releaseNotes = obj["body"].toString();
    
    // Find the Windows executable in assets
    QJsonArray assets = obj["assets"].toArray();
    for (const auto& asset : assets) {
        QJsonObject assetObj = asset.toObject();
        QString name = assetObj["name"].toString();
        if (name.contains("QuickSoundSwitcher") && name.endsWith(".exe")) {
            info.downloadUrl = assetObj["browser_download_url"].toString();
            break;
        }
    }
    
    if (info.downloadUrl.isEmpty()) {
        throw std::runtime_error("No Windows executable found in release assets");
    }
}

bool UpdateChecker::isVersionNewer(const QString& current, const QString& available)
{
    // Remove 'v' prefix if present
    QString cleanCurrent = current.startsWith('v') ? current.mid(1) : current;
    QString cleanAvailable = available.startsWith('v') ? available.mid(1) : available;
    
    QVersionNumber currentVersion = QVersionNumber::fromString(cleanCurrent);
    QVersionNumber availableVersion = QVersionNumber::fromString(cleanAvailable);
    
    return QVersionNumber::compare(availableVersion, currentVersion) > 0;
}

QString UpdateChecker::getCurrentVersion() const
{
    return APP_VERSION_STRING;
}

void UpdateChecker::checkForUpdatesManually()
{
    qDebug() << "Manual update check requested";
    emit manualCheckStarted();
    checkForUpdates();
}

void UpdateChecker::onUpdateCheckFinished(QNetworkReply* reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Update check failed:" << reply->errorString();
        emit updateCheckFailed(reply->errorString());
        emit manualCheckCompleted(false); // Add this
        return;
    }

    QByteArray data = reply->readAll();
    VersionInfo info;

    try {
        parseGitHubResponse(data, info);
        info.isNewer = isVersionNewer(getCurrentVersion(), info.version);

        qDebug() << "Current version:" << getCurrentVersion();
        qDebug() << "Available version:" << info.version;
        qDebug() << "Is newer:" << info.isNewer;

        emit updateCheckFinished(info.isNewer, info);
        emit manualCheckCompleted(info.isNewer); // Add this

        if (info.isNewer) {
            emit updateAvailable(info);
        }
    } catch (const std::exception& e) {
        QString error = QString("Failed to parse response: %1").arg(e.what());
        qDebug() << error;
        emit updateCheckFailed(error);
        emit manualCheckCompleted(false); // Add this
    }
}

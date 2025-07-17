#include "soundpanelbridge.h"
#include "utils.h"
#include "shortcutmanager.h"
#include <QBuffer>
#include <QPixmap>
#include <algorithm>
#include "version.h"
#include "mediasessionmanager.h"
#include "languages.h"

SoundPanelBridge* SoundPanelBridge::m_instance = nullptr;

SoundPanelBridge::SoundPanelBridge(QObject* parent)
    : QObject(parent)
    , settings("Odizinne", "QuickSoundSwitcher")
    , m_currentPanelMode(0)
    , translator(new QTranslator(this))
    , m_chatMixValue(50)
    , m_chatMixCheckTimer(new QTimer(this))
    , m_networkManager(new QNetworkAccessManager(this))
    , m_totalDownloads(0)
    , m_completedDownloads(0)
    , m_failedDownloads(0)
    , m_autoUpdateTimer(new QTimer(this))
{
    m_instance = this;

    loadCommAppsFromFile();
    m_chatMixValue = settings.value("chatMixValue", 50).toInt();

    changeApplicationLanguage(settings.value("languageIndex", 0).toInt());

    // Setup ChatMix monitoring timer
    m_chatMixCheckTimer->setInterval(500);
    connect(m_chatMixCheckTimer, &QTimer::timeout, this, &SoundPanelBridge::checkAndApplyChatMixToNewApps);

    if (settings.value("chatMixEnabled").toBool()) {
        startChatMixMonitoring();
    }

    if (MediaSessionManager::getWorker()) {
        connect(MediaSessionManager::getWorker(), &MediaWorker::mediaInfoChanged,
                this, [this](const MediaInfo& info) {
                    m_mediaTitle = info.title;
                    m_mediaArtist = info.artist;
                    m_mediaArt = info.albumArt;
                    m_isMediaPlaying = info.isPlaying;
                    emit mediaInfoChanged();
                });
    }

    m_autoUpdateTimer->setInterval(4 * 60 * 60 * 1000);
    m_autoUpdateTimer->setSingleShot(false);
    connect(m_autoUpdateTimer, &QTimer::timeout, this, &SoundPanelBridge::checkForTranslationUpdates);
    m_autoUpdateTimer->start();

    if (settings.value("autoUpdateTranslations", false).toBool()) {
        QTimer::singleShot(5000, this, &SoundPanelBridge::checkForTranslationUpdates);
    }
}

SoundPanelBridge::~SoundPanelBridge()
{
    if (m_instance == this) {
        m_instance = nullptr;
    }
}

SoundPanelBridge* SoundPanelBridge::create(QQmlEngine* qmlEngine, QJSEngine* jsEngine)
{
    Q_UNUSED(qmlEngine)
    Q_UNUSED(jsEngine)

    if (!m_instance) {
        m_instance = new SoundPanelBridge();
    }
    return m_instance;
}

SoundPanelBridge* SoundPanelBridge::instance()
{
    return m_instance;
}

void SoundPanelBridge::checkForTranslationUpdates()
{
    if (!settings.value("autoUpdateTranslations", false).toBool()) {
        m_autoUpdateTimer->stop();
        return;
    }

    if (m_activeDownloads.isEmpty()) {
        downloadLatestTranslations();
    }
}

bool SoundPanelBridge::getShortcutState()
{
    return ShortcutManager::isShortcutPresent("QuickSoundSwitcher.lnk");
}

void SoundPanelBridge::setStartupShortcut(bool enabled)
{
    ShortcutManager::manageShortcut(enabled, "QuickSoundSwitcher.lnk");
}

int SoundPanelBridge::panelMode() const
{
    return settings.value("panelMode", 0).toInt();
}

// ChatMix methods
int SoundPanelBridge::chatMixValue() const
{
    return m_chatMixValue;
}

QStringList SoundPanelBridge::getCommAppsFromSettings() const
{
    QStringList result;
    for (const CommApp& app : m_commApps) {
        result.append(app.name);
    }
    return result;
}

bool SoundPanelBridge::isCommApp(const QString& name) const
{
    QStringList commApps = getCommAppsFromSettings();
    return commApps.contains(name, Qt::CaseInsensitive);
}

void SoundPanelBridge::applyChatMixToApplications()
{
    // Note: This now needs to work with AudioBridge instead of direct audio control
    // The actual application volume setting should be handled by the AudioBridge
    // This method can still handle the logic of determining which apps need what volume

    int nonCommAppVolume = m_chatMixValue;

    // Emit signal to let AudioBridge handle the actual volume setting
    // or use AudioBridge directly here

    startChatMixMonitoring();
}

void SoundPanelBridge::checkAndApplyChatMixToNewApps()
{
    bool chatMixEnabled = settings.value("chatMixEnabled", false).toBool();
    if (!chatMixEnabled) {
        return;
    }

    applyChatMixToApplications();
}

void SoundPanelBridge::startChatMixMonitoring()
{
    bool chatMixEnabled = settings.value("chatMixEnabled", false).toBool();
    if (chatMixEnabled && !m_chatMixCheckTimer->isActive()) {
        m_chatMixCheckTimer->start();
    }
}

void SoundPanelBridge::stopChatMixMonitoring()
{
    if (m_chatMixCheckTimer->isActive()) {
        m_chatMixCheckTimer->stop();
    }
}

void SoundPanelBridge::refreshPanelModeState()
{
    emit panelModeChanged();
}

bool SoundPanelBridge::getDarkMode() {
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", QSettings::NativeFormat);
    return settings.value("AppsUseLightTheme", 1).toInt() == 0;
}

QString SoundPanelBridge::taskbarPosition() const
{
    return detectTaskbarPosition();
}

QString SoundPanelBridge::detectTaskbarPosition() const
{
    switch (settings.value("panelPosition", 1).toInt()) {
    case 0:
        return "top";
    case 1:
        return "bottom";
    case 2:
        return "left";
    case 3:
        return "right";
    default:
        return "bottom";
    }
}

QString SoundPanelBridge::getAppVersion() const
{
    return APP_VERSION_STRING;
}

QString SoundPanelBridge::getQtVersion() const
{
    return QT_VERSION_STRING;
}

QString SoundPanelBridge::mediaTitle() const {
    return m_mediaTitle;
}

QString SoundPanelBridge::mediaArtist() const {
    return m_mediaArtist;
}

bool SoundPanelBridge::isMediaPlaying() const {
    return m_isMediaPlaying;
}

QString SoundPanelBridge::mediaArt() const {
    return m_mediaArt;
}

void SoundPanelBridge::playPause() {
    MediaSessionManager::playPauseAsync();
}

void SoundPanelBridge::nextTrack() {
    MediaSessionManager::nextTrackAsync();
}

void SoundPanelBridge::previousTrack() {
    MediaSessionManager::previousTrackAsync();
}

void SoundPanelBridge::startMediaMonitoring() {
    if (settings.value("displayMediaInfos", true).toBool()) {
        MediaSessionManager::startMonitoringAsync();
    }
}

void SoundPanelBridge::stopMediaMonitoring() {
    MediaSessionManager::stopMonitoringAsync();
}

QString SoundPanelBridge::getCurrentLanguageCode() const {
    QLocale locale;
    QString languageCode = locale.name().section('_', 0, 0);

    if (languageCode == "zh") {
        QString fullLocale = locale.name();
        if (fullLocale.startsWith("zh_CN")) {
            return "zh_CN";
        }
        return "zh_CN";
    }

    return languageCode;
}

void SoundPanelBridge::changeApplicationLanguage(int languageIndex)
{
    qApp->removeTranslator(translator);
    delete translator;
    translator = new QTranslator(this);

    QString languageCode;
    if (languageIndex == 0) {
        languageCode = getCurrentLanguageCode();
    } else {
        languageCode = getLanguageCodeFromIndex(languageIndex);
    }

    QString translationFile = QString("./i18n/QuickSoundSwitcher_%1.qm").arg(languageCode);
    if (translator->load(translationFile)) {
        qGuiApp->installTranslator(translator);
    } else {
        qWarning() << "Failed to load translation file:" << translationFile;
    }

    emit languageChanged();
}

QString SoundPanelBridge::getLanguageCodeFromIndex(int index) const
{
    if (index == 0) {
        QLocale systemLocale;
        QString langCode = systemLocale.name().section('_', 0, 0);

        if (langCode == "zh") {
            QString fullLocale = systemLocale.name();
            if (fullLocale.startsWith("zh_CN")) {
                return "zh_CN";
            }
            return "zh_CN";
        }

        return langCode;
    }

    auto languages = getSupportedLanguages();
    if (index > 0 && index <= languages.size()) {
        return languages[index - 1].code;
    }

    return "en";
}

QString SoundPanelBridge::getCommitHash() const
{
    return QString(GIT_COMMIT_HASH);
}

QString SoundPanelBridge::getBuildTimestamp() const
{
    return QString(BUILD_TIMESTAMP);
}

void SoundPanelBridge::setChatMixValue(int value)
{
    if (m_chatMixValue != value) {
        bool wasEnabled = settings.value("chatMixEnabled", false).toBool();
        m_chatMixValue = value;
        emit chatMixValueChanged();

        if (wasEnabled) {
            applyChatMixToApplications();
        }
    }
}

QString SoundPanelBridge::getCommAppsFilePath() const
{
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appDataPath);
    return appDataPath + "/commapps.json";
}

void SoundPanelBridge::loadCommAppsFromFile()
{
    QString filePath = getCommAppsFilePath();
    QFile file(filePath);

    if (!file.exists()) {
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open commapps.json for reading";
        return;
    }

    QByteArray data = file.readAll();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse commapps.json:" << error.errorString();
        return;
    }

    QJsonObject root = doc.object();
    QJsonArray commAppsArray = root["commApps"].toArray();

    m_commApps.clear();
    for (const QJsonValue& value : commAppsArray) {
        QJsonObject appObj = value.toObject();
        CommApp app;
        app.name = appObj["name"].toString();
        app.icon = appObj["icon"].toString();
        m_commApps.append(app);
    }
}

void SoundPanelBridge::saveCommAppsToFile()
{
    QString filePath = getCommAppsFilePath();
    QFile file(filePath);

    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open commapps.json for writing";
        return;
    }

    QJsonArray commAppsArray;
    for (const CommApp& app : m_commApps) {
        QJsonObject appObj;
        appObj["name"] = app.name;
        appObj["icon"] = app.icon;
        commAppsArray.append(appObj);
    }

    QJsonObject root;
    root["commApps"] = commAppsArray;

    QJsonDocument doc(root);
    file.write(doc.toJson());
}

void SoundPanelBridge::addCommApp(const QString& name)
{
    for (const CommApp& existing : m_commApps) {
        if (existing.name.compare(name, Qt::CaseInsensitive) == 0) {
            return;
        }
    }

    CommApp newApp;
    newApp.name = name;

    // Note: Icon retrieval now needs to work with AudioBridge applications
    // This might need to be updated to get icons from AudioBridge.applications

    m_commApps.append(newApp);
    saveCommAppsToFile();
    emit commAppsListChanged();
}

void SoundPanelBridge::removeCommApp(const QString& name)
{
    for (int i = 0; i < m_commApps.count(); ++i) {
        if (m_commApps[i].name.compare(name, Qt::CaseInsensitive) == 0) {
            m_commApps.removeAt(i);
            saveCommAppsToFile();
            emit commAppsListChanged();
            return;
        }
    }
}

void SoundPanelBridge::restoreOriginalVolumes()
{
    stopChatMixMonitoring();

    int restoredVolume = settings.value("chatmixRestoreVolume", 80).toInt();

    // Note: This now needs to work with AudioBridge instead of direct control
    // The actual volume restoration should be handled by AudioBridge
}

QVariantList SoundPanelBridge::commAppsList() const
{
    QVariantList result;
    for (const CommApp& app : m_commApps) {
        QVariantMap appMap;
        appMap["name"] = app.name;
        appMap["icon"] = app.icon;
        result.append(appMap);
    }
    return result;
}

void SoundPanelBridge::updateMissingCommAppIcons()
{
    // Note: This now needs to work with AudioBridge.applications
    // instead of the old applications list
    bool iconsUpdated = false;

    // This method will need to be updated to work with AudioBridge
    // For now, just emit the signal
    if (iconsUpdated) {
        saveCommAppsToFile();
        emit commAppsListChanged();
    }
}

void SoundPanelBridge::toggleChatMixFromShortcut(bool enabled)
{
    emit chatMixEnabledChanged(enabled);
}

void SoundPanelBridge::suspendGlobalShortcuts()
{
    m_globalShortcutsSuspended = true;
}

void SoundPanelBridge::resumeGlobalShortcuts()
{
    m_globalShortcutsSuspended = false;
}

bool SoundPanelBridge::areGlobalShortcutsSuspended() const
{
    return m_globalShortcutsSuspended;
}

void SoundPanelBridge::requestChatMixNotification(QString message) {
    emit chatMixNotificationRequested(message);
}

// Legacy compatibility method
void SoundPanelBridge::onOutputSliderReleased()
{
    // Keep the sound notification for compatibility
    Utils::playSoundNotification();
}

// Translation methods (keeping all of these as they're not audio-related)
void SoundPanelBridge::downloadLatestTranslations()
{
    cancelTranslationDownload();

    m_totalDownloads = 0;
    m_completedDownloads = 0;
    m_failedDownloads = 0;

    QStringList languageCodes = getLanguageCodes();
    QString baseUrl = "https://github.com/Odizinne/QuickSoundSwitcher/raw/refs/heads/main/i18n/compiled/QuickSoundSwitcher_%1.qm";

    m_totalDownloads = languageCodes.size();
    emit translationDownloadStarted();

    for (const QString& langCode : languageCodes) {
        QString url = baseUrl.arg(langCode);
        downloadTranslationFile(langCode, url);
    }

    if (m_totalDownloads == 0) {
        emit translationDownloadFinished(false, tr("No translation files to download"));
    }
}

void SoundPanelBridge::cancelTranslationDownload()
{
    for (QNetworkReply* reply : m_activeDownloads) {
        if (reply && reply->isRunning()) {
            reply->abort();
        }
    }
    m_activeDownloads.clear();
}

void SoundPanelBridge::downloadTranslationFile(const QString& languageCode, const QString& githubUrl)
{
    QUrl url(githubUrl);
    QNetworkRequest request;
    request.setUrl(url);

    QString userAgent = QString("QuickSoundSwitcher/%1").arg(APP_VERSION_STRING);
    request.setRawHeader("User-Agent", userAgent.toUtf8());
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);
    request.setRawHeader("Accept", "*/*");
    request.setRawHeader("Connection", "keep-alive");
    request.setTransferTimeout(30000);

    QNetworkReply* reply = m_networkManager->get(request);
    reply->setProperty("languageCode", languageCode);
    m_activeDownloads.append(reply);

    connect(reply, &QNetworkReply::downloadProgress, this,
            [this, languageCode](qint64 bytesReceived, qint64 bytesTotal) {
                emit translationDownloadProgress(languageCode,
                                                 static_cast<int>(bytesReceived),
                                                 static_cast<int>(bytesTotal));
            });

    connect(reply, &QNetworkReply::finished, this, &SoundPanelBridge::onTranslationFileDownloaded);

    connect(reply, &QNetworkReply::errorOccurred, this,
            [this, languageCode](QNetworkReply::NetworkError error) {
                qWarning() << "Network error for" << languageCode << ":" << error;
            });
}

void SoundPanelBridge::onTranslationFileDownloaded()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    QString languageCode = reply->property("languageCode").toString();
    m_activeDownloads.removeAll(reply);

    if (reply->error() == QNetworkReply::NoError) {
        QString downloadPath = getTranslationDownloadPath();
        QString fileName = QString("QuickSoundSwitcher_%1.qm").arg(languageCode);
        QString filePath = downloadPath + "/" + fileName;

        QDir().mkpath(downloadPath);

        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly)) {
            QByteArray data = reply->readAll();
            if (!data.isEmpty()) {
                file.write(data);
                file.close();
            } else {
                qWarning() << "Downloaded empty file for:" << languageCode;
                m_failedDownloads++;
            }
        } else {
            qWarning() << "Failed to save translation file:" << filePath;
            m_failedDownloads++;
        }
    } else {
        qWarning() << "Failed to download translation for" << languageCode
                   << "Error:" << reply->error()
                   << "HTTP Status:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()
                   << "Message:" << reply->errorString();
        m_failedDownloads++;
    }

    m_completedDownloads++;
    emit translationFileCompleted(languageCode, m_completedDownloads, m_totalDownloads);

    if (m_completedDownloads >= m_totalDownloads) {
        bool success = (m_failedDownloads == 0);
        QString message;

        if (success) {
            message = tr("All translations downloaded successfully");
            changeApplicationLanguage(settings.value("languageIndex", 0).toInt());
        } else {
            message = tr("Downloaded %1 of %2 translation files")
            .arg(m_totalDownloads - m_failedDownloads)
                .arg(m_totalDownloads);
        }

        emit translationDownloadFinished(success, message);

        if (success) {
            int currentLangIndex = settings.value("languageIndex", 0).toInt();
            changeApplicationLanguage(currentLangIndex);
        }
    }

    reply->deleteLater();
}

QString SoundPanelBridge::getTranslationDownloadPath() const
{
    QString appDir = QCoreApplication::applicationDirPath();
    return appDir + "/i18n";
}

QStringList SoundPanelBridge::getLanguageNativeNames() const
{
    auto names = ::getLanguageNativeNames();
    return names;
}

QStringList SoundPanelBridge::getLanguageCodes() const
{
    return ::getLanguageCodes();
}

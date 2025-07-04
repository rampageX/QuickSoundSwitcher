#include "soundpanelbridge.h"
#include "utils.h"
#include "shortcutmanager.h"
#include <QBuffer>
#include <QPixmap>
#include <algorithm>
#include "version.h"
#include "translation_progress.h"
#include "mediasessionmanager.h"
#include <QNetworkRequest>
#include <QProcess>
#include <QCoreApplication>

SoundPanelBridge* SoundPanelBridge::m_instance = nullptr;

SoundPanelBridge::SoundPanelBridge(QObject* parent)
    : QObject(parent)
    , settings("Odizinne", "QuickSoundSwitcher")
    , systemSoundsMuted(false)
    , m_playbackDevicesReady(false)
    , m_recordingDevicesReady(false)
    , m_applicationsReady(false)
    , m_currentPanelMode(0)
    , m_isInitializing(false)
    , translator(new QTranslator(this))
    , m_chatMixValue(50)
    , m_chatMixCheckTimer(new QTimer(this))
    , m_networkManager(new QNetworkAccessManager(this))
    , m_updateTimer(new QTimer(this))
    , m_updateCheckInProgress(false)
{
    m_instance = this;

    loadCommAppsFromFile();
    m_chatMixValue = settings.value("chatMixValue", 50).toInt();

    changeApplicationLanguage(settings.value("languageIndex", 0).toInt());
    populateApplications();

    m_chatMixCheckTimer->setInterval(500);
    connect(m_chatMixCheckTimer, &QTimer::timeout, this, &SoundPanelBridge::checkAndApplyChatMixToNewApps);

    if (settings.value("chatMixEnabled").toBool()) {
        startChatMixMonitoring();
    }

    // Setup update timer - check every 4 hours
    m_updateTimer->setInterval(4 * 60 * 60 * 1000);
    m_updateTimer->setSingleShot(false);
    connect(m_updateTimer, &QTimer::timeout, this, &SoundPanelBridge::performPeriodicCheck);
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &SoundPanelBridge::onUpdateCheckFinished);

    // Connect to audio worker signals for async operations
    if (AudioManager::getWorker()) {
        connect(AudioManager::getWorker(), &AudioWorker::playbackDevicesReady,
                this, [this](const QList<AudioDevice>& devices) {
                    playbackDevices = devices;
                    emit playbackDevicesChanged(convertDevicesToVariant(devices));
                    m_playbackDevicesReady = true;
                    if (m_isInitializing) {
                        checkDataInitializationComplete();
                    }
                });

        connect(AudioManager::getWorker(), &AudioWorker::recordingDevicesReady,
                this, [this](const QList<AudioDevice>& devices) {
                    recordingDevices = devices;
                    emit recordingDevicesChanged(convertDevicesToVariant(devices));
                    m_recordingDevicesReady = true;
                    if (m_isInitializing) {
                        checkDataInitializationComplete();
                    }
                });

        connect(AudioManager::getWorker(), &AudioWorker::applicationsReady,
                this, [this](const QList<Application>& apps) {
                    applications = apps;
                    emit applicationsChanged(convertApplicationsToVariant(apps));
                    m_applicationsReady = true;

                    checkAndApplyChatMixToNewApps();

                    if (m_isInitializing) {
                        checkDataInitializationComplete();
                    }
                });

        connect(AudioManager::getWorker(), &AudioWorker::playbackVolumeChanged,
                this, &SoundPanelBridge::setPlaybackVolume);

        connect(AudioManager::getWorker(), &AudioWorker::recordingVolumeChanged,
                this, &SoundPanelBridge::setRecordingVolume);

        connect(AudioManager::getWorker(), &AudioWorker::playbackMuteChanged,
                this, &SoundPanelBridge::setPlaybackMuted);

        connect(AudioManager::getWorker(), &AudioWorker::recordingMuteChanged,
                this, &SoundPanelBridge::setRecordingMuted);

        connect(AudioManager::getWorker(), &AudioWorker::defaultEndpointChanged,
                this, [this](bool success) {
                    m_deviceChangeInProgress = false;
                    emit deviceChangeInProgressChanged();
                });

        connect(AudioManager::getWorker(), &AudioWorker::currentPropertiesReady,
                this, [this](int playbackVol, int recordingVol, bool playbackMute, bool recordingMute) {
                    setPlaybackVolume(playbackVol);
                    setRecordingVolume(recordingVol);
                    setPlaybackMuted(playbackMute);
                    setRecordingMuted(recordingMute);
                    m_currentPropertiesReady = true;
                    if (m_isInitializing) {
                        checkDataInitializationComplete();
                    }
                });

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

        connect(AudioManager::getWorker(), &AudioWorker::applicationAudioLevelsChanged,
                this, [this](const QList<QPair<QString, int>>& levels) {
                    m_applicationAudioLevels.clear();
                    for (const auto& pair : levels) {
                        m_applicationAudioLevels[pair.first] = pair.second;
                    }
                    emit applicationAudioLevelsChanged();
                });

        connect(AudioManager::getWorker(), &AudioWorker::playbackAudioLevel,
                this, [this](int level) {
                    if (m_playbackAudioLevel != level) {
                        m_playbackAudioLevel = level;
                        emit playbackAudioLevelChanged();
                    }
                });

        connect(AudioManager::getWorker(), &AudioWorker::recordingAudioLevel,
                this, [this](int level) {
                    if (m_recordingAudioLevel != level) {
                        m_recordingAudioLevel = level;
                        emit recordingAudioLevelChanged();
                    }
                });
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

// Update methods
void SoundPanelBridge::startPeriodicUpdateCheck()
{
    if (settings.value("autoUpdateCheck", true).toBool()) {
        m_updateTimer->start();
    }
}

void SoundPanelBridge::setAutoUpdateCheck(bool enabled)
{
    if (enabled) {
        m_updateTimer->start();
    } else {
        m_updateTimer->stop();
    }
}

bool SoundPanelBridge::updateCheckInProgress() const
{
    return m_updateCheckInProgress;
}

void SoundPanelBridge::checkForUpdates()
{
    if (m_updateCheckInProgress) {
        return;
    }

    qDebug() << "Checking for updates...";
    m_updateCheckInProgress = true;
    emit updateCheckInProgressChanged();

    QNetworkRequest request;
    request.setUrl(QUrl("https://api.github.com/repos/Odizinne/QuickSoundSwitcher/releases/latest"));
    request.setRawHeader("User-Agent", "QuickSoundSwitcher-UpdateChecker");
    request.setRawHeader("Accept", "application/vnd.github.v3+json");

    m_networkManager->get(request);
}

void SoundPanelBridge::onUpdateCheckFinished(QNetworkReply* reply)
{
    reply->deleteLater();
    m_updateCheckInProgress = false;
    emit updateCheckInProgressChanged();

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Update check failed:" << reply->errorString();
        emit updateCheckCompleted(false, "");
        return;
    }

    QByteArray data = reply->readAll();

    try {
        parseGitHubResponse(data);
        bool hasUpdate = isVersionNewer(getCurrentVersion(), m_pendingUpdateVersion);

        qDebug() << "Current version:" << getCurrentVersion();
        qDebug() << "Available version:" << m_pendingUpdateVersion;
        qDebug() << "Has update:" << hasUpdate;

        emit updateCheckCompleted(hasUpdate, m_pendingUpdateVersion);

        bool testMode = true;
        if (hasUpdate || testMode) {
            settings.setValue("pendingUpdate/version", m_pendingUpdateVersion);
            settings.setValue("pendingUpdate/downloadUrl", m_pendingUpdateUrl);
            qDebug() << m_pendingUpdateUrl;
            emit updateAvailable(m_pendingUpdateVersion, m_pendingUpdateUrl);
        }
    } catch (const std::exception& e) {
        qDebug() << "Failed to parse response:" << e.what();
        emit updateCheckCompleted(false, "");
    }
}

void SoundPanelBridge::performPeriodicCheck()
{
    checkForUpdates();
}

void SoundPanelBridge::startUpdate()
{
    QString version = settings.value("pendingUpdate/version").toString();
    QString downloadUrl = settings.value("pendingUpdate/downloadUrl").toString();

    if (downloadUrl.isEmpty()) {
        qDebug() << "No update URL available";
        return;
    }

    qDebug() << "Starting update to version:" << version;

    QString updaterPath = QCoreApplication::applicationDirPath() + "/QSSUpdater.exe";
    QStringList arguments;
    arguments << downloadUrl << QCoreApplication::applicationFilePath();

    if (QProcess::startDetached(updaterPath, arguments)) {
        qDebug() << "Updater launched successfully";
        QCoreApplication::quit();
    } else {
        qDebug() << "Failed to start updater";
    }
}

void SoundPanelBridge::parseGitHubResponse(const QByteArray& data)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        throw std::runtime_error(error.errorString().toStdString());
    }

    QJsonObject obj = doc.object();
    m_pendingUpdateVersion = obj["tag_name"].toString();

    QJsonArray assets = obj["assets"].toArray();
    m_pendingUpdateUrl.clear();

    for (const auto& asset : assets) {
        QJsonObject assetObj = asset.toObject();
        QString name = assetObj["name"].toString();
        // Look for zip file instead of exe
        if (name.contains("QuickSoundSwitcher") && name.endsWith(".zip")) {
            m_pendingUpdateUrl = assetObj["browser_download_url"].toString();
            break;
        }
    }

    if (m_pendingUpdateUrl.isEmpty()) {
        throw std::runtime_error("No Windows zip file found in release assets");
    }
}

bool SoundPanelBridge::isVersionNewer(const QString& current, const QString& available)
{
    QString cleanCurrent = current.startsWith('v') ? current.mid(1) : current;
    QString cleanAvailable = available.startsWith('v') ? available.mid(1) : available;

    QVersionNumber currentVersion = QVersionNumber::fromString(cleanCurrent);
    QVersionNumber availableVersion = QVersionNumber::fromString(cleanAvailable);

    return QVersionNumber::compare(availableVersion, currentVersion) > 0;
}

QString SoundPanelBridge::getCurrentVersion() const
{
    return APP_VERSION_STRING;
}

// All existing methods remain the same...
bool SoundPanelBridge::getShortcutState()
{
    return ShortcutManager::isShortcutPresent("QuickSoundSwitcher.lnk");
}

void SoundPanelBridge::setStartupShortcut(bool enabled)
{
    ShortcutManager::manageShortcut(enabled, "QuickSoundSwitcher.lnk");
}

int SoundPanelBridge::playbackVolume() const
{
    return m_playbackVolume;
}

void SoundPanelBridge::setPlaybackVolume(int volume)
{
    if (m_playbackVolume != volume) {
        m_playbackVolume = volume;
        emit playbackVolumeChanged();
    }
}

int SoundPanelBridge::recordingVolume() const
{
    return m_recordingVolume;
}

void SoundPanelBridge::setRecordingVolume(int volume)
{
    if (m_recordingVolume != volume) {
        m_recordingVolume = volume;
        emit recordingVolumeChanged();
    }
}

bool SoundPanelBridge::playbackMuted() const
{
    return m_playbackMuted;
}

void SoundPanelBridge::setPlaybackMuted(bool muted)
{
    if (m_playbackMuted != muted) {
        m_playbackMuted = muted;
        emit playbackMutedChanged();
    }
}

bool SoundPanelBridge::recordingMuted() const
{
    return m_recordingMuted;
}

void SoundPanelBridge::setRecordingMuted(bool muted)
{
    if (m_recordingMuted != muted) {
        m_recordingMuted = muted;
        emit recordingMutedChanged();
    }
}

int SoundPanelBridge::panelMode() const
{
    return settings.value("panelMode", 0).toInt();
}

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
    int nonCommAppVolume = m_chatMixValue;

    for (const Application& app : applications) {
        bool isComm = isCommApp(app.name);
        int targetVolume = isComm ? 100 : nonCommAppVolume;

        bool shouldGroup = settings.value("groupApplications", true).toBool();
        if (shouldGroup) {
            QStringList appIDs = app.id.split(";");
            for (const QString& id : appIDs) {
                AudioManager::setApplicationVolumeAsync(id, targetVolume);
            }
        } else {
            AudioManager::setApplicationVolumeAsync(app.id, targetVolume);
        }
    }

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

void SoundPanelBridge::initializeData() {
    m_isInitializing = true;
    int mode = panelMode();
    m_currentPanelMode = mode;

    m_playbackDevicesReady = false;
    m_recordingDevicesReady = false;
    m_applicationsReady = false;
    m_currentPropertiesReady = false;

    if (mode == 0 || mode == 2) {
        populatePlaybackDevices();
        populateRecordingDevices();
        AudioManager::queryCurrentPropertiesAsync();
    } else {
        AudioManager::queryCurrentPropertiesAsync();
        m_playbackDevicesReady = true;
        m_recordingDevicesReady = true;
    }

    if (mode == 0 || mode == 1) {
        populateApplications();
    } else {
        m_applicationsReady = true;
    }

    m_mediaTitle = "";
    m_mediaArtist = "";
    m_mediaArt = "";
    m_isMediaPlaying = false;
    emit mediaInfoChanged();
    if (settings.value("mediaMode", true).toInt() != 2) {
        MediaSessionManager::queryMediaInfoAsync();
    }

    bool chatMixEnabled = settings.value("chatMixEnabled", false).toBool();
    if (chatMixEnabled) {
        startChatMixMonitoring();
    } else {
        stopChatMixMonitoring();
    }

    checkDataInitializationComplete();
}

void SoundPanelBridge::refreshData()
{
    initializeData();
}

void SoundPanelBridge::populatePlaybackDevices()
{
    AudioManager::enumeratePlaybackDevicesAsync();
}

void SoundPanelBridge::populateRecordingDevices()
{
    AudioManager::enumerateRecordingDevicesAsync();
}

void SoundPanelBridge::populateApplications()
{
    AudioManager::enumerateApplicationsAsync();
}

QVariantList SoundPanelBridge::convertDevicesToVariant(const QList<AudioDevice>& devices)
{
    QVariantList deviceList;
    for (const AudioDevice& device : devices) {
        QVariantMap deviceMap;
        deviceMap["id"] = device.id;
        deviceMap["name"] = device.name;
        deviceMap["shortName"] = device.shortName.isEmpty() ? device.name : device.shortName;
        deviceMap["type"] = device.type;
        deviceMap["isDefault"] = device.isDefault;
        deviceList.append(deviceMap);
    }
    return deviceList;
}

QVariantList SoundPanelBridge::convertApplicationsToVariant(const QList<Application>& apps)
{
    QVariantList applicationList;
    bool shouldGroup = settings.value("groupApplications", true).toBool();

    if (!shouldGroup) {
        QList<Application> filteredApps;
        QList<Application> systemSoundApps;

        for (const Application &app : apps) {
            if (app.name.isEmpty() ||
                app.name.compare("QuickSoundSwitcher", Qt::CaseInsensitive) == 0) {
                continue;
            }

            if (app.name == "Windows system sounds") {
                systemSoundApps.append(app);
            } else {
                filteredApps.append(app);
            }
        }

        std::sort(filteredApps.begin(), filteredApps.end(), [](const Application &a, const Application &b) {
            QString nameA = a.name.isEmpty() ? a.name : a.name;
            QString nameB = b.name.isEmpty() ? b.name : b.name;
            return nameA.compare(nameB, Qt::CaseInsensitive) < 0;
        });

        for (const Application &app : filteredApps) {
            QPixmap iconPixmap = app.icon.pixmap(16, 16);
            QByteArray byteArray;
            QBuffer buffer(&byteArray);
            buffer.open(QIODevice::WriteOnly);
            iconPixmap.toImage().save(&buffer, "PNG");
            QString base64Icon = QString::fromUtf8(byteArray.toBase64());

            QVariantMap appMap;
            appMap["appID"] = app.id;
            appMap["name"] = app.name.isEmpty() ? app.name : app.name;
            appMap["isMuted"] = app.isMuted;
            appMap["volume"] = app.volume;
            appMap["icon"] = "data:image/png;base64," + base64Icon;
            appMap["audioLevel"] = app.audioLevel;

            applicationList.append(appMap);
        }

        for (const Application &app : systemSoundApps) {
            QPixmap iconPixmap = app.icon.pixmap(16, 16);
            QByteArray byteArray;
            QBuffer buffer(&byteArray);
            buffer.open(QIODevice::WriteOnly);
            iconPixmap.toImage().save(&buffer, "PNG");
            QString base64Icon = QString::fromUtf8(byteArray.toBase64());

            QVariantMap appMap;
            appMap["appID"] = app.id;
            appMap["name"] = app.name;
            appMap["isMuted"] = app.isMuted;
            appMap["volume"] = app.volume;
            appMap["icon"] = "data:image/png;base64," + base64Icon;
            appMap["audioLevel"] = app.audioLevel;

            applicationList.append(appMap);
        }

        return applicationList;
    }

    QMap<QString, QVector<Application>> groupedApps;
    QVector<Application> systemSoundApps;

    for (const Application &app : apps) {
        if (app.name.isEmpty() ||
            app.name.compare("QuickSoundSwitcher", Qt::CaseInsensitive) == 0) {
            continue;
        }

        if (app.name == "Windows system sounds") {
            systemSoundApps.append(app);
            continue;
        }
        groupedApps[app.name].append(app);
    }

    for (auto it = groupedApps.begin(); it != groupedApps.end(); ++it) {
        QString executableName = it.key();
        QVector<Application> appGroup = it.value();

        if (appGroup.isEmpty()) {
            continue;
        }

        QString displayName = appGroup[0].name.isEmpty() ? executableName : appGroup[0].name;

        bool isMuted = std::any_of(appGroup.begin(), appGroup.end(), [](const Application &app) {
            return app.isMuted;
        });

        int volume = 0;
        if (!appGroup.isEmpty()) {
            volume = std::max_element(appGroup.begin(), appGroup.end(), [](const Application &a, const Application &b) {
                         return a.volume < b.volume;
                     })->volume;
        }

        int maxAudioLevel = 0;
        if (!appGroup.isEmpty()) {
            maxAudioLevel = std::max_element(appGroup.begin(), appGroup.end(), [](const Application &a, const Application &b) {
                                return a.audioLevel < b.audioLevel;
                            })->audioLevel;
        }

        QPixmap iconPixmap = appGroup[0].icon.pixmap(16, 16);
        QByteArray byteArray;
        QBuffer buffer(&byteArray);
        buffer.open(QIODevice::WriteOnly);
        iconPixmap.toImage().save(&buffer, "PNG");
        QString base64Icon = QString::fromUtf8(byteArray.toBase64());

        QStringList appIDs;
        for (const Application &app : appGroup) {
            appIDs.append(app.id);
        }

        QVariantMap appMap;
        appMap["appID"] = appIDs.join(";");
        appMap["name"] = displayName;
        appMap["isMuted"] = isMuted;
        appMap["volume"] = volume;
        appMap["icon"] = "data:image/png;base64," + base64Icon;
        appMap["audioLevel"] = maxAudioLevel;

        applicationList.append(appMap);
    }

    if (!systemSoundApps.isEmpty()) {
        QString displayName = systemSoundApps[0].name.isEmpty() ? "Windows system sounds" : systemSoundApps[0].name;

        bool isMuted = std::any_of(systemSoundApps.begin(), systemSoundApps.end(), [](const Application &app) {
            return app.isMuted;
        });

        int volume = 0;
        if (!systemSoundApps.isEmpty()) {
            volume = std::max_element(systemSoundApps.begin(), systemSoundApps.end(), [](const Application &a, const Application &b) {
                         return a.volume < b.volume;
                     })->volume;
        }

        int maxAudioLevel = 0;
        if (!systemSoundApps.isEmpty()) {
            maxAudioLevel = std::max_element(systemSoundApps.begin(), systemSoundApps.end(), [](const Application &a, const Application &b) {
                                return a.audioLevel < b.audioLevel;
                            })->audioLevel;
        }

        QPixmap iconPixmap = systemSoundApps[0].icon.pixmap(16, 16);
        QByteArray byteArray;
        QBuffer buffer(&byteArray);
        buffer.open(QIODevice::WriteOnly);
        iconPixmap.toImage().save(&buffer, "PNG");
        QString base64Icon = QString::fromUtf8(byteArray.toBase64());

        QStringList appIDs;
        for (const Application &app : systemSoundApps) {
            appIDs.append(app.id);
        }

        QVariantMap appMap;
        appMap["appID"] = appIDs.join(";");
        appMap["name"] = displayName;
        appMap["isMuted"] = isMuted;
        appMap["volume"] = volume;
        appMap["icon"] = "data:image/png;base64," + base64Icon;
        appMap["audioLevel"] = maxAudioLevel;

        applicationList.append(appMap);

        systemSoundsMuted = isMuted;
    }

    return applicationList;
}

void SoundPanelBridge::onPlaybackVolumeChanged(int volume)
{
    AudioManager::setPlaybackVolumeAsync(volume);
    if (AudioManager::getPlaybackMute()) {
        AudioManager::setPlaybackMuteAsync(false);
    }
    emit shouldUpdateTray();
}

void SoundPanelBridge::onRecordingVolumeChanged(int volume)
{
    AudioManager::setRecordingVolumeAsync(volume);
}

void SoundPanelBridge::onOutputMuteButtonClicked()
{
    bool isMuted = AudioManager::getPlaybackMute();
    AudioManager::setPlaybackMuteAsync(!isMuted);
    emit shouldUpdateTray();
}

void SoundPanelBridge::onInputMuteButtonClicked()
{
    bool isMuted = AudioManager::getRecordingMute();
    AudioManager::setRecordingMuteAsync(!isMuted);
}

void SoundPanelBridge::onOutputSliderReleased()
{
    if (!systemSoundsMuted) {
        Utils::playSoundNotification();
    }
}

void SoundPanelBridge::onApplicationVolumeSliderValueChanged(QString appID, int volume)
{
    bool shouldGroup = settings.value("groupApplications", true).toBool();
    bool chatMixEnabled = settings.value("chatMixEnabled", false).toBool();

    if (chatMixEnabled) {
        return;
    }

    if (shouldGroup) {
        QStringList appIDs = appID.split(";");
        for (const QString& id : appIDs) {
            AudioManager::setApplicationVolumeAsync(id, volume);
        }
    } else {
        AudioManager::setApplicationVolumeAsync(appID, volume);
    }
}

void SoundPanelBridge::onApplicationMuteButtonClicked(QString appID, bool state)
{
    if (appID == "0") {
        systemSoundsMuted = state;
    }

    bool shouldGroup = settings.value("groupApplications", true).toBool();

    if (shouldGroup) {
        QStringList appIDs = appID.split(";");
        for (const QString& id : appIDs) {
            AudioManager::setApplicationMuteAsync(id, state);
        }
    } else {
        AudioManager::setApplicationMuteAsync(appID, state);
    }
}

void SoundPanelBridge::updateVolumeFromTray(int volume)
{
    setPlaybackVolume(volume);
}

void SoundPanelBridge::updateMuteStateFromTray(bool muted)
{
    setPlaybackMuted(muted);
}

void SoundPanelBridge::refreshPanelModeState()
{
    emit panelModeChanged();
}

void SoundPanelBridge::checkDataInitializationComplete() {
    bool needsDevices = (m_currentPanelMode == 0 || m_currentPanelMode == 2);
    bool needsApplications = (m_currentPanelMode == 0 || m_currentPanelMode == 1);

    bool devicesReady = !needsDevices || (m_playbackDevicesReady && m_recordingDevicesReady);
    bool applicationsReady = !needsApplications || m_applicationsReady;
    bool propertiesReady = m_currentPropertiesReady;

    if (devicesReady && applicationsReady && propertiesReady) {
        m_isInitializing = false;
        emit dataInitializationComplete();
    }
}

bool SoundPanelBridge::getDarkMode() {
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", QSettings::NativeFormat);
    return settings.value("AppsUseLightTheme", 1).toInt() == 0;
}

bool SoundPanelBridge::deviceChangeInProgress() const
{
    return m_deviceChangeInProgress;
}

void SoundPanelBridge::onPlaybackDeviceChanged(const QString &deviceName)
{
    m_deviceChangeInProgress = true;
    emit deviceChangeInProgressChanged();

    for (const AudioDevice& device : playbackDevices) {
        if (device.shortName == deviceName || device.name == deviceName) {
            AudioManager::setDefaultEndpointAsync(device.id);
            break;
        }
    }
}

void SoundPanelBridge::onRecordingDeviceChanged(const QString &deviceName)
{
    m_deviceChangeInProgress = true;
    emit deviceChangeInProgressChanged();

    for (const AudioDevice& device : recordingDevices) {
        if (device.shortName == deviceName || device.name == deviceName) {
            AudioManager::setDefaultEndpointAsync(device.id);
            break;
        }
    }
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

int SoundPanelBridge::getTotalTranslatableStrings() const {
    auto allProgress = getTranslationProgressMap();

    for (auto it = allProgress.begin(); it != allProgress.end(); ++it) {
        QVariantMap langData = it.value().toMap();
        return langData["total"].toInt();
    }

    return 0;
}

int SoundPanelBridge::getCurrentLanguageFinishedStrings(int languageIndex) const {
    QString currentLang = getLanguageCodeFromIndex(languageIndex);
    auto allProgress = getTranslationProgressMap();

    if (allProgress.contains(currentLang)) {
        QVariantMap langData = allProgress[currentLang].toMap();
        return langData["finished"].toInt();
    }

    return 0;
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

    QString translationFile = QString(":/i18n/QuickSoundSwitcher_%1.qm").arg(languageCode);
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
        return systemLocale.name().left(2);
    }

    switch (index) {
    case 1: return "en";
    case 2: return "fr";
    case 3: return "de";
    case 4: return "it";
    case 5: return "ko";
    case 6: return "zh_CN";
    default: return "en";
    }
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

    for (const Application& app : applications) {
        if (app.name.compare(name, Qt::CaseInsensitive) == 0) {
            QPixmap iconPixmap = app.icon.pixmap(16, 16);
            QByteArray byteArray;
            QBuffer buffer(&byteArray);
            buffer.open(QIODevice::WriteOnly);
            iconPixmap.toImage().save(&buffer, "PNG");
            QString base64Icon = QString::fromUtf8(byteArray.toBase64());
            newApp.icon = "data:image/png;base64," + base64Icon;
            break;
        }
    }

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
    for (Application& app : applications) {
        bool shouldGroup = settings.value("groupApplications", true).toBool();
        if (shouldGroup) {
            QStringList appIDs = app.id.split(";");
            for (const QString& id : appIDs) {
                AudioManager::setApplicationVolumeAsync(id, restoredVolume);
            }
        } else {
            AudioManager::setApplicationVolumeAsync(app.id, restoredVolume);
        }

        app.volume = restoredVolume;
    }

    emit applicationsChanged(convertApplicationsToVariant(applications));
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
    bool iconsUpdated = false;

    for (CommApp& commApp : m_commApps) {
        if (!commApp.icon.isEmpty()) {
            continue;
        }

        for (const Application& app : applications) {
            if (app.name.compare(commApp.name, Qt::CaseInsensitive) == 0) {
                QPixmap iconPixmap = app.icon.pixmap(16, 16);
                if (!iconPixmap.isNull()) {
                    QByteArray byteArray;
                    QBuffer buffer(&byteArray);
                    buffer.open(QIODevice::WriteOnly);
                    iconPixmap.toImage().save(&buffer, "PNG");
                    QString base64Icon = QString::fromUtf8(byteArray.toBase64());
                    commApp.icon = "data:image/png;base64," + base64Icon;
                    iconsUpdated = true;
                }
                break;
            }
        }
    }

    if (iconsUpdated) {
        saveCommAppsToFile();
        emit commAppsListChanged();
    }
}

QVariantList SoundPanelBridge::applicationAudioLevels() const {
    QVariantList result;
    for (auto it = m_applicationAudioLevels.begin(); it != m_applicationAudioLevels.end(); ++it) {
        QVariantMap item;
        item["appId"] = it.key();
        item["level"] = it.value();
        result.append(item);
    }
    return result;
}

int SoundPanelBridge::playbackAudioLevel() const {
    return m_playbackAudioLevel;
}

int SoundPanelBridge::recordingAudioLevel() const {
    return m_recordingAudioLevel;
}

void SoundPanelBridge::startAudioLevelMonitoring() {
    AudioManager::startAudioLevelMonitoring();
}

void SoundPanelBridge::stopAudioLevelMonitoring() {
    AudioManager::stopAudioLevelMonitoring();
}

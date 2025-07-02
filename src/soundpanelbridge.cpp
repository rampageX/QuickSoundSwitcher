#include "soundpanelbridge.h"
#include "utils.h"
#include "shortcutmanager.h"
#include <QBuffer>
#include <QPixmap>
#include <algorithm>
#include "version.h"
#include "translation_progress.h"
#include "mediasessionmanager.h"

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
{
    m_instance = this;

    changeApplicationLanguage(settings.value("languageIndex", 0).toInt());

    // Connect to audio worker signals for async operations
    if (AudioManager::getWorker()) {
        connect(AudioManager::getWorker(), &AudioWorker::playbackDevicesReady,
                this, [this](const QList<AudioDevice>& devices) {
                    playbackDevices = devices;
                    emit playbackDevicesChanged(convertDevicesToVariant(devices));
                    m_playbackDevicesReady = true;
                    if (m_isInitializing) {  // Only check completion during initialization
                        checkDataInitializationComplete();
                    }
                });

        connect(AudioManager::getWorker(), &AudioWorker::recordingDevicesReady,
                this, [this](const QList<AudioDevice>& devices) {
                    recordingDevices = devices;
                    emit recordingDevicesChanged(convertDevicesToVariant(devices));
                    m_recordingDevicesReady = true;
                    if (m_isInitializing) {  // Only check completion during initialization
                        checkDataInitializationComplete();
                    }
                });

        connect(AudioManager::getWorker(), &AudioWorker::applicationsReady,
                this, [this](const QList<Application>& apps) {
                    applications = apps;
                    emit applicationsChanged(convertApplicationsToVariant(apps));
                    m_applicationsReady = true;
                    if (m_isInitializing) {  // Only check completion during initialization
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
                    if (m_isInitializing) {  // Only check completion during initialization
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

void SoundPanelBridge::initializeData() {
    m_isInitializing = true;  // Set flag to indicate we're initializing
    int mode = panelMode();
    m_currentPanelMode = mode;

    m_playbackDevicesReady = false;
    m_recordingDevicesReady = false;
    m_applicationsReady = false;
    m_currentPropertiesReady = false;

    if (mode == 0 || mode == 2) {  // devices + mixer OR devices only
        populatePlaybackDevices();
        populateRecordingDevices();
        AudioManager::queryCurrentPropertiesAsync();
    } else {
        AudioManager::queryCurrentPropertiesAsync();
        m_playbackDevicesReady = true;
        m_recordingDevicesReady = true;
    }

    if (mode == 0 || mode == 1) {  // devices + mixer OR mixer only
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
        // Show individual sinks without grouping, but sorted alphabetically
        QList<Application> filteredApps;
        QList<Application> systemSoundApps;

        // First filter and separate system sounds from regular apps
        for (const Application &app : apps) {
            if (app.executableName.isEmpty() ||
                app.executableName.compare("QuickSoundSwitcher", Qt::CaseInsensitive) == 0) {
                continue;
            }

            if (app.name == "Windows system sounds") {
                systemSoundApps.append(app);
            } else {
                filteredApps.append(app);
            }
        }

        // Sort regular apps alphabetically by display name
        std::sort(filteredApps.begin(), filteredApps.end(), [](const Application &a, const Application &b) {
            QString nameA = a.name.isEmpty() ? a.executableName : a.name;
            QString nameB = b.name.isEmpty() ? b.executableName : b.name;
            return nameA.compare(nameB, Qt::CaseInsensitive) < 0;
        });

        // Convert regular apps to variant list
        for (const Application &app : filteredApps) {
            QPixmap iconPixmap = app.icon.pixmap(16, 16);
            QByteArray byteArray;
            QBuffer buffer(&byteArray);
            buffer.open(QIODevice::WriteOnly);
            iconPixmap.toImage().save(&buffer, "PNG");
            QString base64Icon = QString::fromUtf8(byteArray.toBase64());

            QVariantMap appMap;
            appMap["appID"] = app.id;
            appMap["name"] = app.name.isEmpty() ? app.executableName : app.name;
            appMap["isMuted"] = app.isMuted;
            appMap["volume"] = app.volume;
            appMap["icon"] = "data:image/png;base64," + base64Icon;

            applicationList.append(appMap);
        }

        // Add system sounds at the end (each individual sink)
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

            applicationList.append(appMap);
        }

        return applicationList;
    }

    // Original grouping logic (naturally sorted by QMap keys)
    QMap<QString, QVector<Application>> groupedApps;
    QVector<Application> systemSoundApps;

    // Group applications by executable name
    for (const Application &app : apps) {
        if (app.executableName.isEmpty() ||
            app.executableName.compare("QuickSoundSwitcher", Qt::CaseInsensitive) == 0) {
            continue;
        }

        if (app.name == "Windows system sounds") {
            systemSoundApps.append(app);
            continue;
        }
        groupedApps[app.executableName].append(app);
    }

    // Process grouped applications
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

        applicationList.append(appMap);
    }

    // Process system sounds (grouped version - at the end)
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

        applicationList.append(appMap);

        systemSoundsMuted = isMuted;
    }

    return applicationList;
}

void SoundPanelBridge::onPlaybackVolumeChanged(int volume)
{
    AudioManager::setPlaybackVolumeAsync(volume);
    // Use cached getter (non-blocking)
    if (AudioManager::getPlaybackMute()) {
        AudioManager::setPlaybackMuteAsync(false);
        // Don't call setPlaybackMuted(false) here - let the signal handle it
    }
    // Remove this line: setPlaybackVolume(volume);
    emit shouldUpdateTray();
}

void SoundPanelBridge::onRecordingVolumeChanged(int volume)
{
    AudioManager::setRecordingVolumeAsync(volume);
    // Remove this line: setRecordingVolume(volume);
}

void SoundPanelBridge::onOutputMuteButtonClicked()
{
    // Use cached getter (non-blocking)
    bool isMuted = AudioManager::getPlaybackMute();
    AudioManager::setPlaybackMuteAsync(!isMuted);
    // Remove this line: setPlaybackMuted(!isMuted);
    emit shouldUpdateTray();
}

void SoundPanelBridge::onInputMuteButtonClicked()
{
    // Use cached getter (non-blocking)
    bool isMuted = AudioManager::getRecordingMute();
    AudioManager::setRecordingMuteAsync(!isMuted);
    // Remove this line: setRecordingMuted(!isMuted);
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

    if (shouldGroup) {
        QStringList appIDs = appID.split(";");
        for (const QString& id : appIDs) {
            AudioManager::setApplicationVolumeAsync(id, volume);
        }
    } else {
        // Single app ID when not grouping
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
        m_isInitializing = false;  // Reset flag when initialization is complete
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

    // Handle Chinese variants specifically
    if (languageCode == "zh") {
        QString fullLocale = locale.name();
        if (fullLocale.startsWith("zh_CN")) {
            return "zh_CN";
        }
        return "zh_CN"; // Default to simplified Chinese
    }

    return languageCode;
}

int SoundPanelBridge::getTotalTranslatableStrings() const {
    auto allProgress = getTranslationProgressMap();

    // Just get the total from any language (they should all be the same)
    for (auto it = allProgress.begin(); it != allProgress.end(); ++it) {
        QVariantMap langData = it.value().toMap();
        return langData["total"].toInt();
    }

    return 0; // Fallback
}

int SoundPanelBridge::getCurrentLanguageFinishedStrings(int languageIndex) const {
    QString currentLang = getLanguageCodeFromIndex(languageIndex);
    auto allProgress = getTranslationProgressMap();

    if (allProgress.contains(currentLang)) {
        QVariantMap langData = allProgress[currentLang].toMap();
        return langData["finished"].toInt();
    }

    return 0; // Fallback
}

void SoundPanelBridge::changeApplicationLanguage(int languageIndex)
{
    qApp->removeTranslator(translator);
    delete translator;
    translator = new QTranslator(this);

    QString languageCode;
    if (languageIndex == 0) {
        QLocale systemLocale;
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

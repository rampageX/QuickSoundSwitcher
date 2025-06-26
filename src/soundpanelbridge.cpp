#include "soundpanelbridge.h"
#include "utils.h"
#include "shortcutmanager.h"
#include <QBuffer>
#include <QPixmap>
#include <algorithm>

SoundPanelBridge* SoundPanelBridge::m_instance = nullptr;

SoundPanelBridge::SoundPanelBridge(QObject* parent)
    : QObject(parent)
    , settings("Odizinne", "QuickSoundSwitcher")
    , systemSoundsMuted(false)
    , m_playbackDevicesReady(false)
    , m_recordingDevicesReady(false)
    , m_applicationsReady(false)
    , m_currentPanelMode(0)
{
    m_instance = this;

    // Connect to audio worker signals for async operations
    if (AudioManager::getWorker()) {
        connect(AudioManager::getWorker(), &AudioWorker::playbackDevicesReady,
                this, [this](const QList<AudioDevice>& devices) {
                    playbackDevices = devices;
                    emit playbackDevicesChanged(convertDevicesToVariant(devices));
                    m_playbackDevicesReady = true;
                    checkDataInitializationComplete();
                });

        connect(AudioManager::getWorker(), &AudioWorker::recordingDevicesReady,
                this, [this](const QList<AudioDevice>& devices) {
                    recordingDevices = devices;
                    emit recordingDevicesChanged(convertDevicesToVariant(devices));
                    m_recordingDevicesReady = true;
                    checkDataInitializationComplete();
                });

        connect(AudioManager::getWorker(), &AudioWorker::applicationsReady,
                this, [this](const QList<Application>& apps) {
                    applications = apps;
                    emit applicationsChanged(convertApplicationsToVariant(apps));
                    m_applicationsReady = true;
                    checkDataInitializationComplete();
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

void SoundPanelBridge::initializeData()
{
    int mode = panelMode();
    m_currentPanelMode = mode;

    // Reset ready flags
    m_playbackDevicesReady = false;
    m_recordingDevicesReady = false;
    m_applicationsReady = false;

    if (mode == 0 || mode == 2) {  // devices + mixer OR devices only
        populatePlaybackDevices();
        populateRecordingDevices();

        // Set initial values from cached state (non-blocking)
        setPlaybackVolume(AudioManager::getPlaybackVolume());
        setRecordingVolume(AudioManager::getRecordingVolume());
        setPlaybackMuted(AudioManager::getPlaybackMute());
        setRecordingMuted(AudioManager::getRecordingMute());
    } else {
        // If we don't need devices, mark them as ready
        m_playbackDevicesReady = true;
        m_recordingDevicesReady = true;
    }

    if (mode == 0 || mode == 1) {  // devices + mixer OR mixer only
        populateApplications();
    } else {
        // If we don't need applications, mark as ready
        m_applicationsReady = true;
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

    QVariantList applicationList;

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

    // Process system sounds
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
    QStringList appIDs = appID.split(";");
    for (const QString& id : appIDs) {
        AudioManager::setApplicationVolumeAsync(id, volume);
    }
}

void SoundPanelBridge::onApplicationMuteButtonClicked(QString appID, bool state)
{
    if (appID == "0") {
        systemSoundsMuted = state;
    }

    QStringList appIDs = appID.split(";");
    for (const QString& id : appIDs) {
        AudioManager::setApplicationMuteAsync(id, state);
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

void SoundPanelBridge::checkDataInitializationComplete()
{
    bool needsDevices = (m_currentPanelMode == 0 || m_currentPanelMode == 2);
    bool needsApplications = (m_currentPanelMode == 0 || m_currentPanelMode == 1);

    bool devicesReady = !needsDevices || (m_playbackDevicesReady && m_recordingDevicesReady);
    bool applicationsReady = !needsApplications || m_applicationsReady;

    if (devicesReady && applicationsReady) {
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

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
{
    m_instance = this;
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
    if (mode == 0 || mode == 2) {  // devices + mixer OR devices only
        populatePlaybackDevices();
        populateRecordingDevices();
        setPlaybackVolume(AudioManager::getPlaybackVolume());
        setRecordingVolume(AudioManager::getRecordingVolume());
        setPlaybackMuted(AudioManager::getPlaybackMute());
        setRecordingMuted(AudioManager::getRecordingMute());
    }
    if (mode == 0 || mode == 1) {  // devices + mixer OR mixer only
        populateApplications();
    }
}

void SoundPanelBridge::refreshData()
{
    initializeData();
}

void SoundPanelBridge::populatePlaybackDevices()
{
    playbackDevices.clear();
    AudioManager::enumeratePlaybackDevices(playbackDevices);
    emit playbackDevicesChanged(convertDevicesToVariant(playbackDevices));
}

void SoundPanelBridge::populateRecordingDevices()
{
    recordingDevices.clear();
    AudioManager::enumerateRecordingDevices(recordingDevices);
    emit recordingDevicesChanged(convertDevicesToVariant(recordingDevices));
}

void SoundPanelBridge::populateApplications()
{
    applications = AudioManager::enumerateAudioApplications();
    emit applicationsChanged(convertApplicationsToVariant(applications));
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
    AudioManager::setPlaybackVolume(volume);
    if (AudioManager::getPlaybackMute()) {
        AudioManager::setPlaybackMute(false);
        setPlaybackMuted(false);
    }
    setPlaybackVolume(volume);
    emit shouldUpdateTray();
}

void SoundPanelBridge::onRecordingVolumeChanged(int volume)
{
    AudioManager::setRecordingVolume(volume);
    setRecordingVolume(volume);
}

void SoundPanelBridge::onPlaybackDeviceChanged(const QString &deviceName)
{
    for (const AudioDevice& device : playbackDevices) {
        if (device.shortName == deviceName || device.name == deviceName) {
            AudioManager::setDefaultEndpoint(device.id);
            setPlaybackVolume(AudioManager::getPlaybackVolume());
            setPlaybackMuted(AudioManager::getPlaybackMute());
            break;
        }
    }
}

void SoundPanelBridge::onRecordingDeviceChanged(const QString &deviceName)
{
    for (const AudioDevice& device : recordingDevices) {
        if (device.shortName == deviceName || device.name == deviceName) {
            AudioManager::setDefaultEndpoint(device.id);
            setRecordingVolume(AudioManager::getRecordingVolume());
            setRecordingMuted(AudioManager::getRecordingMute());
            break;
        }
    }
}

void SoundPanelBridge::onOutputMuteButtonClicked()
{
    bool isMuted = AudioManager::getPlaybackMute();
    AudioManager::setPlaybackMute(!isMuted);
    setPlaybackMuted(!isMuted);
    emit shouldUpdateTray();
}

void SoundPanelBridge::onInputMuteButtonClicked()
{
    bool isMuted = AudioManager::getRecordingMute();
    AudioManager::setRecordingMute(!isMuted);
    setRecordingMuted(!isMuted);
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
        AudioManager::setApplicationVolume(id, volume);
    }
}

void SoundPanelBridge::onApplicationMuteButtonClicked(QString appID, bool state)
{
    if (appID == "0") {
        systemSoundsMuted = state;
    }

    QStringList appIDs = appID.split(";");
    for (const QString& id : appIDs) {
        AudioManager::setApplicationMute(id, state);
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

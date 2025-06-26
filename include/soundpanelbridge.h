#ifndef SOUNDPANELBRIDGE_H
#define SOUNDPANELBRIDGE_H

#include "audiomanager.h"
#include <QObject>
#include <QQmlEngine>
#include <QtQml/qqmlregistration.h>
#include <QSettings>

class SoundPanelBridge : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(int playbackVolume READ playbackVolume WRITE setPlaybackVolume NOTIFY playbackVolumeChanged)
    Q_PROPERTY(int recordingVolume READ recordingVolume WRITE setRecordingVolume NOTIFY recordingVolumeChanged)
    Q_PROPERTY(bool playbackMuted READ playbackMuted WRITE setPlaybackMuted NOTIFY playbackMutedChanged)
    Q_PROPERTY(bool recordingMuted READ recordingMuted WRITE setRecordingMuted NOTIFY recordingMutedChanged)
    Q_PROPERTY(int panelMode READ panelMode NOTIFY panelModeChanged)
    Q_PROPERTY(bool deviceChangeInProgress READ deviceChangeInProgress NOTIFY deviceChangeInProgressChanged)

public:
    explicit SoundPanelBridge(QObject* parent = nullptr);
    ~SoundPanelBridge() override;

    // Singleton factory function
    static SoundPanelBridge* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);
    static SoundPanelBridge* instance();

    int playbackVolume() const;
    void setPlaybackVolume(int volume);

    int recordingVolume() const;
    void setRecordingVolume(int volume);

    bool playbackMuted() const;
    void setPlaybackMuted(bool muted);

    bool recordingMuted() const;
    void setRecordingMuted(bool muted);

    int panelMode() const;

    Q_INVOKABLE void initializeData();
    Q_INVOKABLE void refreshData();
    Q_INVOKABLE bool getShortcutState();
    Q_INVOKABLE void setStartupShortcut(bool enabled);
    Q_INVOKABLE bool getDarkMode();
    bool deviceChangeInProgress() const;

public slots:
    void onPlaybackVolumeChanged(int volume);
    void onRecordingVolumeChanged(int volume);
    void onPlaybackDeviceChanged(const QString &deviceName);
    void onRecordingDeviceChanged(const QString &deviceName);
    void onOutputMuteButtonClicked();
    void onInputMuteButtonClicked();
    void onOutputSliderReleased();
    void onApplicationVolumeSliderValueChanged(QString appID, int volume);
    void onApplicationMuteButtonClicked(QString appID, bool state);

    // External updates (called from QuickSoundSwitcher)
    void updateVolumeFromTray(int volume);
    void updateMuteStateFromTray(bool muted);
    void refreshPanelModeState();

signals:
    void playbackVolumeChanged();
    void recordingVolumeChanged();
    void playbackMutedChanged();
    void recordingMutedChanged();
    void panelModeChanged();
    void shouldUpdateTray();
    void playbackDevicesChanged(const QVariantList& devices);
    void recordingDevicesChanged(const QVariantList& devices);
    void applicationsChanged(const QVariantList& applications);
    void dataInitializationComplete();
    void deviceChangeInProgressChanged();

private:
    static SoundPanelBridge* m_instance;
    QList<AudioDevice> playbackDevices;
    QList<AudioDevice> recordingDevices;
    QList<Application> applications;
    int m_playbackVolume = 0;
    int m_recordingVolume = 0;
    bool m_playbackMuted = false;
    bool m_recordingMuted = false;
    QSettings settings;
    bool systemSoundsMuted;

    void populatePlaybackDevices();
    void populateRecordingDevices();
    void populateApplications();
    QVariantList convertDevicesToVariant(const QList<AudioDevice>& devices);
    QVariantList convertApplicationsToVariant(const QList<Application>& apps);

    bool m_playbackDevicesReady = false;
    bool m_recordingDevicesReady = false;
    bool m_applicationsReady = false;
    int m_currentPanelMode = 0;

    void checkDataInitializationComplete();
    bool m_deviceChangeInProgress = false;
};

#endif // SOUNDPANELBRIDGE_H

#ifndef SOUNDPANELBRIDGE_H
#define SOUNDPANELBRIDGE_H

#include "audiomanager.h"
#include <QObject>
#include <QQmlEngine>
#include <QtQml/qqmlregistration.h>
#include <QSettings>
#include <QScreen>
#include <QGuiApplication>
#include <QTranslator>

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
    Q_PROPERTY(QString taskbarPosition READ taskbarPosition NOTIFY taskbarPositionChanged)

    Q_PROPERTY(QString mediaTitle READ mediaTitle NOTIFY mediaInfoChanged)
    Q_PROPERTY(QString mediaArtist READ mediaArtist NOTIFY mediaInfoChanged)
    Q_PROPERTY(bool isMediaPlaying READ isMediaPlaying NOTIFY mediaInfoChanged)
    Q_PROPERTY(QString mediaArt READ mediaArt NOTIFY mediaInfoChanged)

public:
    explicit SoundPanelBridge(QObject* parent = nullptr);
    ~SoundPanelBridge() override;

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
    QString taskbarPosition() const;

    Q_INVOKABLE void initializeData();
    Q_INVOKABLE void refreshData();
    Q_INVOKABLE bool getShortcutState();
    Q_INVOKABLE void setStartupShortcut(bool enabled);
    Q_INVOKABLE bool getDarkMode();
    Q_INVOKABLE QString getAppVersion() const;
    Q_INVOKABLE QString getQtVersion() const;
    Q_INVOKABLE QString getCommitHash() const;
    Q_INVOKABLE QString getBuildTimestamp() const;

    QString mediaTitle() const;
    QString mediaArtist() const;
    bool isMediaPlaying() const;
    QString mediaArt() const;

    bool deviceChangeInProgress() const;

    Q_INVOKABLE void playPause();
    Q_INVOKABLE void nextTrack();
    Q_INVOKABLE void previousTrack();
    Q_INVOKABLE void startMediaMonitoring();
    Q_INVOKABLE void stopMediaMonitoring();

    Q_INVOKABLE int getTotalTranslatableStrings() const;
    Q_INVOKABLE QString getCurrentLanguageCode() const;
    Q_INVOKABLE int getCurrentLanguageFinishedStrings(int languageIndex) const;
    Q_INVOKABLE void changeApplicationLanguage(int languageIndex);
    Q_INVOKABLE QString getLanguageCodeFromIndex(int index) const;


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
    void taskbarPositionChanged();
    void mediaInfoChanged();
    void languageChanged();

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
    bool m_currentPropertiesReady = false;
    bool m_applicationsReady = false;
    bool m_isInitializing = false;  // Add this line
    int m_currentPanelMode = 0;

    void checkDataInitializationComplete();
    bool m_deviceChangeInProgress = false;

    QString detectTaskbarPosition() const;
    QString m_mediaTitle;
    QString m_mediaArtist;
    bool m_isMediaPlaying = false;
    QString m_mediaArt;

    QTranslator *translator;
};

#endif // SOUNDPANELBRIDGE_H

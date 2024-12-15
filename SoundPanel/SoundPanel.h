#ifndef MEDIASFLYOUT_H
#define MEDIASFLYOUT_H

#include "AudioManager.h"
#include <QObject>
#include <QQmlApplicationEngine>
#include <QWindow>
#include <QIcon>
#include <QComboBox>

class SoundPanel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int playbackVolume READ playbackVolume WRITE setPlaybackVolume NOTIFY playbackVolumeChanged)
    Q_PROPERTY(int recordingVolume READ recordingVolume WRITE setRecordingVolume NOTIFY recordingVolumeChanged)

public:
    explicit SoundPanel(QObject* parent = nullptr);
    ~SoundPanel() override;

    QWindow* soundPanelWindow;

    int playbackVolume() const;
    void setPlaybackVolume(int volume);

    int recordingVolume() const;
    void setRecordingVolume(int volume);

    void setOutputButtonImage(int volume);
    void setInputButtonImage(bool muted);

    void setSystemSoundsIcon();

    void animateOut();

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

private slots:
    void onVolumeChangedWithTray(int volume);
    void onOutputMuteStateChanged(int volumeIcon);

signals:
    void playbackVolumeChanged();
    void recordingVolumeChanged();

    void shouldUpdateTray();
    void panelClosed();

private:
    QQmlApplicationEngine* engine;
    QList<AudioDevice> playbackDevices;
    QList<AudioDevice> recordingDevices;
    QList<Application> applications;
    int m_playbackVolume = 0;
    int m_recordingVolume = 0;

    void animateIn();
    void configureQML();
    void setupUI();
    void populateComboBoxes();
    void populateApplicationModel();
    bool isWindows10;
    bool isAnimating;
};

#endif // MEDIASFLYOUT_H

#ifndef MEDIASFLYOUT_H
#define MEDIASFLYOUT_H

#include "AudioManager.h"
#include <QObject>
#include <QQmlApplicationEngine>
#include <QWindow>
#include <QIcon>
#include <QComboBox>

class MediaFlyout : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int playbackVolume READ playbackVolume WRITE setPlaybackVolume NOTIFY playbackVolumeChanged)
    Q_PROPERTY(int recordingVolume READ recordingVolume WRITE setRecordingVolume NOTIFY recordingVolumeChanged)

public:
    explicit MediaFlyout(QObject* parent = nullptr);
    ~MediaFlyout() override;

    bool visible = false;
    bool isAnimating = false;
    void animateIn();
    void animateOut();
    QWindow* mediaFlyoutWindow;

    int playbackVolume() const;
    void setPlaybackVolume(int volume);

    int recordingVolume() const;
    void setRecordingVolume(int volume);

    void populateComboBoxes();
    void setupUI();

    void setOutputButtonImage(int volume);
    void setInputButtonImage(bool muted);

public slots:
    void onPlaybackVolumeChanged(int volume);
    void onRecordingVolumeChanged(int volume);

    void onPlaybackDeviceChanged(const QString &deviceName);
    void onRecordingDeviceChanged(const QString &deviceName);

    void onOutputMuteButtonClicked();
    void onInputMuteButtonClicked();
private slots:

signals:
    void playbackVolumeChanged();
    void recordingVolumeChanged();

    void shouldUpdateTray();

private:
    QQmlApplicationEngine* engine;

    QList<AudioDevice> playbackDevices;
    QList<AudioDevice> recordingDevices;

    int m_playbackVolume = 0;
    int m_recordingVolume = 0;

};

#endif // MEDIASFLYOUT_H

#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include <QString>
#include <QList>
#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>

struct AudioDevice {
    QString id;
    QString name;
    QString shortName;
    QString type;
    bool isDefault;
};

namespace AudioManager {

void initialize();
void cleanup();

void enumeratePlaybackDevices(QList<AudioDevice>& playbackDevices);
void enumerateRecordingDevices(QList<AudioDevice>& recordingDevices);

void setPlaybackVolume(int volume);
int getPlaybackVolume();

void setRecordingVolume(int volume);
int getRecordingVolume();

void setPlaybackMute(bool mute);
bool getPlaybackMute();

void setRecordingMute(bool mute);
bool getRecordingMute();

int getPlaybackAudioLevel();
int getRecordingAudioLevel();

QString extractShortName(const QString& fullName);
}

#endif // AUDIOMANAGER_H

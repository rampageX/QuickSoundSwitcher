#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include <QString>
#include <QList>

namespace AudioManager {

struct AudioDevice {
    QString name;
    QString shortName;
    QString id;
    bool isDefault;
    QString type;
};

QString getAudioDeviceOutput();
void parseAudioDeviceOutput(QList<AudioDevice> &playbackDevices, QList<AudioDevice> &recordingDevices);

void setVolume(int volume, bool playback);
int getVolume(bool playback);
void setMute(bool playback);
bool getMute(bool playback);
}

#endif // AUDIOMANAGER_H

#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include <QString>
#include <QList>

// Define a struct to hold audio device information
namespace AudioManager {

struct AudioDevice {
    QString name;
    QString shortName;
    QString id;
    bool isDefault;
    QString type;  // either "Playback" or "Recording"
};

// Function to get audio device output from PowerShell
QString getAudioDeviceOutput();

// Function to parse audio device output and populate the provided lists
void parseAudioDeviceOutput(QList<AudioDevice> &playbackDevices, QList<AudioDevice> &recordingDevices);

// Example usage function to demonstrate the functionality
void exampleUsage();

} // namespace AudioManager

#endif // AUDIOMANAGER_H

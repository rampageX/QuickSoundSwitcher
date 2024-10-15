#include "AudioManager.h"
#include <QProcess>
#include <QDebug>
#include <QCoreApplication>
#include <QRegularExpression>

namespace AudioManager {

QString getAudioDeviceOutput() {
    QString program = "powershell";
    QStringList arguments;
    arguments << "-Command" << "Get-AudioDevice -l";

    QProcess process;
    process.start(program, arguments);

    if (!process.waitForFinished()) {
        qDebug() << "Error: Could not execute PowerShell command.";
        return QString();
    }

    return process.readAllStandardOutput();
}

QString extractShortName(const QString& fullName) {
    int firstOpenParenIndex = fullName.indexOf('(');
    int lastCloseParenIndex = fullName.lastIndexOf(')');

    if (firstOpenParenIndex != -1 && lastCloseParenIndex != -1 && firstOpenParenIndex < lastCloseParenIndex) {
        return fullName.mid(firstOpenParenIndex + 1, lastCloseParenIndex - firstOpenParenIndex - 1).trimmed();
    }

    return fullName;
}

void parseAudioDeviceOutput(QList<AudioDevice> &playbackDevices, QList<AudioDevice> &recordingDevices) {
    QString output = getAudioDeviceOutput();
    if (output.isEmpty()) {
        qDebug() << "No output from PowerShell command.";
        return;
    }

    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    AudioDevice device;

    for (const QString &line : lines) {
        if (line.startsWith("Index")) {
            // Append device to the correct list if it has a valid name and id
            if (!device.name.isEmpty() && !device.id.isEmpty()) {
                // Extract the short name based on the rules
                device.shortName = extractShortName(device.name);

                // Add the device to the correct list based on type
                if (device.type == "Playback") {
                    playbackDevices.append(device);
                } else if (device.type == "Recording") {
                    recordingDevices.append(device);
                }
                device = AudioDevice(); // Reset for the next device
            }
        } else if (line.startsWith("Name")) {
            device.name = line.section(':', 1).trimmed(); // Keep the full name
        } else if (line.startsWith("ID")) {
            device.id = line.section(':', 1).trimmed();
        } else if (line.startsWith("Default") && !line.startsWith("DefaultCommunication")) {
            QString defaultValue = line.section(':', 1).trimmed();
            device.isDefault = (defaultValue == "True");
        } else if (line.startsWith("Type")) {
            device.type = line.section(':', 1).trimmed();
        }
    }

    // Handle the last device if there is one
    if (!device.name.isEmpty() && !device.id.isEmpty()) {
        device.shortName = extractShortName(device.name); // Extract short name for the last device

        // Add the device to the correct list based on type
        if (device.type == "Playback") {
            playbackDevices.append(device);
        } else if (device.type == "Recording") {
            recordingDevices.append(device);
        }
    }
}

}

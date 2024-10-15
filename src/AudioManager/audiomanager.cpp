#include "AudioManager.h"
#include <QProcess>
#include <QDebug>
#include <QCoreApplication>
#include <QRegularExpression>
#include <QtMath>

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

void setVolume(int volume, bool playback) {
    QString program = "powershell";
    QString command;

    if (playback) {
        command = QString("& { Set-AudioDevice -PlaybackVolume %1 }").arg(volume);
    } else {
        command = QString("& { Set-AudioDevice -RecordingVolume %1 }").arg(volume);
    }

    QStringList arguments;
    arguments << "-Command" << command;

    QProcess process;
    process.start(program, arguments);
    process.waitForFinished();
}

int getVolume(bool playback) {
    QString program = "powershell";
    QStringList arguments;

    if (playback) {
        arguments << "-Command" << "Get-AudioDevice -PlaybackVolume";
    } else {
        arguments << "-Command" << "Get-AudioDevice -RecordingVolume";
    }

    QProcess process;
    process.start(program, arguments);
    process.waitForFinished();

    QString output = process.readAllStandardOutput();

    QRegularExpression regex("(\\d+(?:\\.\\d+)?)%"); // Matches integers or decimals followed by %
    QRegularExpressionMatch match = regex.match(output);

    if (match.hasMatch()) {
        QString volumeStr = match.captured(1);
        bool ok;
        double volume = volumeStr.toDouble(&ok);

        if (ok) {
            return qRound(volume);
        }
    }
    return -1;
}

void setMute(bool playback) {
    QString program = "powershell";
    QString command;

    if (playback) {
        command = QString("& { Set-AudioDevice -PlaybackMuteToggle }");
    } else {
        command = QString("& { Set-AudioDevice -RecordingMuteToggle }");
    }

    QStringList arguments;
    arguments << "-Command" << command;

    QProcess process;
    process.start(program, arguments);
    process.waitForFinished();
}

bool getMute(bool playback) {
    QString program = "powershell";
    QString command;

    if (playback) {
        command = QString("& { Get-AudioDevice -PlaybackMute }");
    } else {
        command = QString("& { Get-AudioDevice -RecordingMute }");
    }

    QStringList arguments;
    arguments << "-Command" << command;

    QProcess process;
    process.start(program, arguments);
    process.waitForFinished();

    QString output = process.readAllStandardOutput().trimmed();

    if (output == "True") {
        return true;
    } else if (output == "False") {
        return false;
    }

    qWarning() << "Unexpected output:" << output;
    return false; // Default to false if the output is unexpected
}

}

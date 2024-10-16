#include "quicksoundswitcher.h"
#include <QApplication>
#include <QMessageBox>
#include <QProcess>
#include <QDebug>

bool isAudioDeviceCmdletsInstalled() {
    QString command = "Get-Command Get-AudioDevice";

    QProcess process;
    process.start("powershell.exe", QStringList() << "-Command" << command);
    process.waitForFinished();

    QString output = process.readAllStandardOutput();

    if (output.contains("AudioDeviceCmdlets")) {
        return true;
    }

    return false;
}

void promptToInstall() {
    QMessageBox msgBox;
    msgBox.setWindowTitle("Audio Device Cmdlets Not Installed");
    msgBox.setText("The AudioDeviceCmdlets PowerShell cmdlet is not installed.\n"
                   "Please install it to continue using this application.");
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    if (!isAudioDeviceCmdletsInstalled()) {
        promptToInstall();
        return 1;
    }

    QuickSoundSwitcher w;
    a.setStyle("fusion");
    a.setQuitOnLastWindowClosed(false);
    return a.exec();
}

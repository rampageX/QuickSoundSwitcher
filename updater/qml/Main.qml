import QtQuick
import QtQuick.Controls.FluentWinUI3
import QtQuick.Layouts

ApplicationWindow {
    id: window
    width: 400
    height: 200
    visible: true
    title: "QuickSoundSwitcher Updater"
    flags: Qt.Dialog

    property alias statusText: statusLabel.text
    property alias progress: progressBar.value

    ColumnLayout {
        anchors.centerIn: parent
        width: parent.width - 60
        spacing: 20

        Label {
            id: statusLabel
            text: "Preparing update..."
            font.pixelSize: 14
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }

        ProgressBar {
            id: progressBar
            Layout.fillWidth: true
            value: 0.0
            from: 0.0
            to: 1.0
        }

        Label {
            text: Math.round(progressBar.value * 100) + "%"
            font.pixelSize: 12
            Layout.alignment: Qt.AlignHCenter
            opacity: 0.7
        }
    }
}


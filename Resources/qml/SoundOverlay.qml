import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.FluentWinUI3 2.15
import QtQuick.Layouts 1.15

ApplicationWindow {
    visible: false
    width: 216
    height: 46
    title: "Sound Overlay"
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    color: "transparent"

    Rectangle {
        anchors.fill: parent
        color: nativeWindowColor
        radius: 8
        border.width: 1
        border.color: Qt.rgba(255, 255, 255, 0.15)
    }

    Frame {
        anchors.fill: parent

        RowLayout {
            anchors.fill: parent

            Image {
                id: volumeIcon
                source: volumeIconPixmap
                width: 16
                height: 16
                Layout.alignment: Qt.AlignVCenter
            }

            ProgressBar {
                id: volumeBar
                value: volumeBarValue
                from: 0
                to: 100
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                Layout.leftMargin: 10
                Layout.rightMargin: 10
            }

            Label {
                id: volumeLabel
                text: volumeLabelText
                visible: true
                font.pixelSize: 13
                Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
                Layout.preferredWidth: 14
            }
        }
    }
}

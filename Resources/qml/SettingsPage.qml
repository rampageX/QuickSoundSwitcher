import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls.FluentWinUI3 2.15

ApplicationWindow {
    visible: false
    width: 360
    height: 150
    minimumWidth: 360
    maximumWidth: 360
    minimumHeight: 150
    maximumHeight: 150
    title: "Settings"

    ColumnLayout {
        id: columnLayout
        anchors.fill: parent
        anchors.leftMargin: 9
        anchors.rightMargin: 9
        anchors.topMargin: 9
        anchors.bottomMargin: 9

        Frame {
            id: frame2
            Layout.fillWidth: true

            GridLayout {
                id: gridLayout2
                anchors.fill: parent

                Label {
                    id: label2
                    font.pixelSize: 13
                    font.bold: true
                    text: qsTr("Volume increment")
                }

                SpinBox {
                    id: spinBox
                    from: 1
                    to: 20
                    value: settingsPage.volumeIncrement
                    Layout.preferredHeight: 32
                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                    onValueChanged: settingsPage.volumeIncrement = value
                }
            }
        }

        Frame {
            id: frame1
            Layout.fillWidth: true

            GridLayout {
                id: gridLayout1
                anchors.fill: parent

                Label {
                    id: label1
                    font.pixelSize: 13
                    font.bold: true
                    text: qsTr("Merge similar apps")
                }

                Switch {
                    id: _soundNotificationSwitch
                    checked: settingsPage.mergeSimilarApps
                    Layout.preferredHeight: 32
                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                    onCheckedChanged: settingsPage.mergeSimilarApps = checked
                }
            }
        }
    }
}

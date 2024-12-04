import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls.FluentWinUI3 2.15

ApplicationWindow {
    visible: false
    width: 360
    height: 146
    minimumWidth: 360
    maximumWidth: 360
    minimumHeight: 146
    maximumHeight: 146
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
                    text: qsTr("Enable mode:")
                }

                ComboBox {
                    id: spinBox
                    Layout.preferredHeight: 32
                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                    //currentIndex: configurator.mode // Bind ComboBox to the mode property

                    // Connect to the C++ function when the index changes
                    //onCurrentIndexChanged: configurator.setMode(currentIndex)
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
                    Layout.preferredHeight: 32
                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                    //checked: configurator.notification // Bind the switch to the C++ property

                    // Connect to the C++ function
                    //onCheckedChanged: configurator.setNotification(checked)
                }
            }
        }
    }
}

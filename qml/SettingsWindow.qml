import QtQuick.Controls.FluentWinUI3
import QtQuick.Layouts
import QtQuick
import Odizinne.QuickSoundSwitcher

ApplicationWindow {
    id: root
    height: mainLyt.implicitHeight + 30
    minimumHeight: mainLyt.implicitHeight + 30
    maximumHeight: mainLyt.implicitHeight + 30
    width: 450
    minimumWidth: 450
    maximumWidth: 450
    visible: false
    transientParent: null
    title: qsTr("QuickSoundSwitcher - Settings")

    property int rowHeight: 35

    ColumnLayout {
        id: mainLyt
        spacing: 15
        anchors.margins: 15
        anchors.fill: parent

        RowLayout {
            spacing: 15
            Layout.preferredHeight: root.rowHeight
            Label {
                text: qsTr("Panel mode")
                Layout.fillWidth: true
            }

            ComboBox {
                Layout.preferredHeight: 35
                Layout.preferredWidth: 150
                model: [qsTr("Devices + Mixer"), qsTr("Mixer only"), qsTr("Devices only")]
                currentIndex: UserSettings.panelMode
                onActivated: UserSettings.panelMode = currentIndex
            }
        }

        RowLayout {
            spacing: 15
            Layout.preferredHeight: root.rowHeight
            Label {
                text: qsTr("Link same input and output devices")
                Layout.fillWidth: true
            }

            Switch {
                checked: UserSettings.linkIO
                onClicked: UserSettings.linkIO = checked
            }
        }

        RowLayout {
            spacing: 15
            Layout.preferredHeight: root.rowHeight
            Label {
                text: qsTr("Sound keepalive")
                Layout.fillWidth: true
            }

            Switch {
                checked: UserSettings.keepAlive
                onClicked: {
                    UserSettings.keepAlive = checked
                }
            }
        }

        RowLayout {
            spacing: 15
            Layout.preferredHeight: root.rowHeight
            Label {
                text: qsTr("Run at system startup")
                Layout.fillWidth: true
            }

            Switch {
                checked: SoundPanelBridge.getShortcutState()
                onClicked: SoundPanelBridge.setStartupShortcut(checked)
            }
        }

        RowLayout {
            spacing: 15
            Layout.preferredHeight: root.rowHeight
            Label {
                text: qsTr("Panel position")
                Layout.fillWidth: true
            }
            ComboBox {
                Layout.preferredHeight: 35
                Layout.preferredWidth: 150
                model: [qsTr("Top"), qsTr("Bottom")]
                currentIndex: UserSettings.panelPosition
                onActivated: UserSettings.panelPosition = currentIndex
            }
        }

        RowLayout {
            spacing: 15
            Layout.preferredHeight: root.rowHeight
            Label {
                text: qsTr("Taskbar offset")
                Layout.fillWidth: true
            }
            SpinBox {
                Layout.preferredHeight: 35
                Layout.preferredWidth: 150
                from: 0
                to: 100
                editable: true
                value: UserSettings.taskbarOffset
                onValueModified: UserSettings.taskbarOffset = value
            }
        }

        RowLayout {
            spacing: 15
            Layout.preferredHeight: root.rowHeight
            Label {
                text: qsTr("Use short device names")
                Layout.fillWidth: true
            }

            Switch {
                checked: UserSettings.deviceShortName
                onClicked: UserSettings.deviceShortName = checked
            }
        }
    }
}

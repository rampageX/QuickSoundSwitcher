import QtQuick.Controls.Material
import QtQuick.Layouts
import Odizinne.QuickSoundSwitcher

ApplicationWindow {
    id: root
    height: mainLyt.implicitHeight + 30
    minimumHeight: mainLyt.implicitHeight + 30
    maximumHeight: mainLyt.implicitHeight + 30
    width: 400
    minimumWidth: 400
    maximumWidth: 400
    visible: false
    transientParent: null
    Material.theme: Material.System
    title: "QuickSoundSwitcher - Settings"
    color: Material.theme === Material.Dark ? "#1C1C1C" : "#E3E3E3"

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
                text: "Volume mixer only"
                Layout.fillWidth: true
            }

            Switch {
                checked: UserSettings.mixerOnly
                onClicked: UserSettings.mixerOnly = checked
            }
        }

        RowLayout {
            spacing: 15
            Layout.preferredHeight: root.rowHeight
            Label {
                text: "Link same input and output devices"
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
                text: "Run at system startup"
                Layout.fillWidth: true
            }

            CheckBox {
                checked: SoundPanelBridge.getShortcutState()
                onClicked: SoundPanelBridge.setStartupShortcut(checked)
            }
        }
    }
}

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.FluentWinUI3
import Odizinne.QuickSoundSwitcher

Rectangle {
    id: root

    property alias model: devicesList.model
    property bool expanded: false
    property real contentOpacity: 0

    property real expandedNeededHeight: devicesList.contentHeight + 20

    signal deviceClicked(string name, int index)

    Layout.fillWidth: true
    Layout.preferredHeight: expanded ? expandedNeededHeight : 0
    Layout.leftMargin: -14
    Layout.rightMargin: -14
    color: Constants.footerColor

    Connections {
        target: AudioBridge
        function onDeviceRenameUpdated() {
            devicesList.forceLayout()
        }
    }

    Behavior on Layout.preferredHeight {
        NumberAnimation {
            duration: 150
            easing.type: Easing.OutQuad
        }
    }

    Rectangle {
        visible: root.expanded
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        color: Constants.footerBorderColor
        height: 1
        opacity: 0.1
    }

    Rectangle {
        visible: root.expanded
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        color: Constants.footerBorderColor
        height: 1
        opacity: 0.1
    }

    ListView {
        id: devicesList
        anchors.fill: parent
        anchors.margins: 10
        clip: true
        interactive: false
        opacity: root.contentOpacity

        Behavior on opacity {
            NumberAnimation {
                duration: 200
                easing.type: Easing.OutQuad
            }
        }

        delegate: ItemDelegate {
            id: del
            width: devicesList.width
            height: 40
            required property var model
            required property string name
            required property string description
            required property bool isDefault
            required property string deviceId
            required property int index

            highlighted: model.isDefault

            text: AudioBridge.getDisplayNameForDevice(model.name || "")

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton | Qt.RightButton

                onClicked: function(mouse) {
                    if (mouse.button === Qt.LeftButton) {
                        root.deviceClicked(del.model.name, del.index)
                    } else if (mouse.button === Qt.RightButton) {
                        deviceRenameContextMenu.originalName = del.model.name || ""
                        deviceRenameContextMenu.currentCustomName = AudioBridge.getCustomDeviceName(del.model.name || "")
                        deviceRenameContextMenu.popup()
                    }
                }
            }
        }
    }

    Timer {
        id: opacityTimer
        interval: 112
        repeat: false
        onTriggered: root.contentOpacity = 1
    }

    onExpandedChanged: {
        if (expanded) {
            if (UserSettings.opacityAnimations) {
                opacityTimer.start()
            } else {
                root.contentOpacity = 1
            }
        } else {
            if (UserSettings.opacityAnimations) {
                root.contentOpacity = 0
            } else {
                root.contentOpacity = 0
            }
        }
    }

    Menu {
        id: deviceRenameContextMenu

        property string originalName: ""
        property string currentCustomName: ""

        MenuItem {
            text: qsTr("Rename Device")
            onTriggered: deviceRenameDialog.open()
        }

        MenuItem {
            text: qsTr("Reset to Original Name")
            enabled: deviceRenameContextMenu.currentCustomName !== deviceRenameContextMenu.originalName
            onTriggered: {
                AudioBridge.setCustomDeviceName(deviceRenameContextMenu.originalName, "")
            }
        }
    }

    Dialog {
        id: deviceRenameDialog
        title: qsTr("Rename Device")
        modal: true
        width: 300
        dim: false
        anchors.centerIn: parent

        ColumnLayout {
            anchors.fill: parent
            spacing: 15

            TextField {
                id: deviceCustomNameField
                Layout.fillWidth: true
                placeholderText: deviceRenameContextMenu.originalName

                Keys.onReturnPressed: {
                    AudioBridge.setCustomDeviceName(
                        deviceRenameContextMenu.originalName,
                        deviceCustomNameField.text.trim()
                    )
                    deviceRenameDialog.close()
                }
            }

            RowLayout {
                spacing: 15
                Layout.topMargin: 10

                Button {
                    text: qsTr("Cancel")
                    onClicked: deviceRenameDialog.close()
                    Layout.fillWidth: true
                }

                Button {
                    text: qsTr("Save")
                    highlighted: true
                    Layout.fillWidth: true
                    onClicked: {
                        AudioBridge.setCustomDeviceName(
                            deviceRenameContextMenu.originalName,
                            deviceCustomNameField.text.trim()
                        )
                        deviceRenameDialog.close()
                    }
                }
            }
        }

        onVisibleChanged: {
            if(!visible) return

            deviceCustomNameField.text = deviceRenameContextMenu.currentCustomName
            deviceCustomNameField.forceActiveFocus()
        }
    }
}

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.FluentWinUI3
import Odizinne.QuickSoundSwitcher

ColumnLayout {
    spacing: 3

    Label {
        text: qsTr("Communication Apps")
        font.pixelSize: 22
        font.bold: true
        Layout.bottomMargin: 15
    }

    Connections {
        target: SoundPanelBridge
        function onApplicationsChanged() {
            SoundPanelBridge.updateMissingCommAppIcons()
        }
    }

    ScrollView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        ColumnLayout {
            width: parent.width
            spacing: 3

            Card {
                Layout.fillWidth: true
                title: qsTr("Enable ChatMix")
                description: qsTr("Control communication apps separately from other applications")

                additionalControl: Switch {
                    checked: UserSettings.chatMixEnabled
                    onClicked: {
                        if (checked) {
                            UserSettings.chatMixEnabled = checked
                            SoundPanelBridge.saveOriginalVolumesAfterRefresh()
                        } else {
                            UserSettings.chatMixEnabled = checked
                            SoundPanelBridge.restoreOriginalVolumes()
                        }
                    }
                }
            }

            Card {
                Layout.fillWidth: true
                title: qsTr("Communication Applications")
                description: qsTr("Add application names that should be treated as communication apps")

                additionalControl: Button {
                    text: qsTr("Add App")
                    onClicked: addAppDialog.open()
                }
            }

            Repeater {
                model: SoundPanelBridge.commAppsList
                Card {
                    id: appCard
                    required property var model
                    Layout.fillWidth: true
                    title: model.name
                    iconSource: model.icon
                    description: qsTr("Original Volume: %1%").arg(model.originalVolume)
                    imageMode: true
                    additionalControl: Button {
                        text: qsTr("Remove")
                        onClicked: {
                            SoundPanelBridge.removeCommApp(appCard.model.name)
                        }
                    }
                }
            }
        }
    }

    Dialog {
        id: addAppDialog
        title: qsTr("Add Communication App")
        modal: true
        width: 350
        anchors.centerIn: parent

        ColumnLayout {
            anchors.fill: parent
            spacing: 15

            Label {
                text: qsTr("Enter application name (e.g., Discord)")
            }

            TextField {
                id: executableField
                Layout.fillWidth: true
                placeholderText: qsTr("Discord")
            }

            RowLayout {
                spacing: 15
                Button {
                    text: qsTr("Cancel")
                    onClicked: addAppDialog.close()
                    Layout.fillWidth: true
                }

                Button {
                    text: qsTr("Add")
                    enabled: executableField.text.length > 0
                    Layout.fillWidth: true
                    highlighted: true
                    onClicked: {
                        SoundPanelBridge.addCommApp(executableField.text)
                        executableField.text = ""
                        addAppDialog.close()
                    }
                }
            }
        }
    }
}

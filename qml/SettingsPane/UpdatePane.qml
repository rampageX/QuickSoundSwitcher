import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.FluentWinUI3
import Odizinne.QuickSoundSwitcher

ColumnLayout {
    id: lyt
    spacing: 3
    property bool hasUpdate: false
    property string updateVersion: ""

    Connections {
        target: SoundPanelBridge
        function onUpdateAvailable(version, downloadUrl) {
            lyt.hasUpdate = true
            lyt.updateVersion = version
        }
    }

    Label {
        text: qsTr("Updates")
        font.pixelSize: 22
        font.bold: true
        Layout.bottomMargin: 15
    }

    ScrollView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        ColumnLayout {
            width: parent.width
            spacing: 3
            // Update available card
            Card {
                Layout.fillWidth: true
                visible: lyt.hasUpdate
                title: qsTr("Update available: %1").arg(lyt.updateVersion)
                description: qsTr("A new version is ready to download and install")

                additionalControl: Button {
                    text: qsTr("Update now")
                    highlighted: true
                    onClicked: {
                        SoundPanelBridge.startUpdate()
                    }
                }
            }

            // Auto update settings
            Card {
                Layout.fillWidth: true
                title: qsTr("Automatic updates")
                description: qsTr("Check for updates every 4 hours automatically")

                additionalControl: Switch {
                    checked: UserSettings.autoUpdateCheck
                    onClicked: {
                        UserSettings.autoUpdateCheck = checked
                        SoundPanelBridge.setAutoUpdateCheck(checked)
                    }
                }
            }

            // Manual update check
            Card {
                Layout.fillWidth: true
                title: qsTr("Check for updates")
                description: qsTr("Manually check for application updates")
                enabled: !lyt.hasUpdate

                additionalControl: Button {
                    text: SoundPanelBridge.updateCheckInProgress ? qsTr("Checking...") : qsTr("Check now")
                    enabled: !SoundPanelBridge.updateCheckInProgress

                    onClicked: {
                        SoundPanelBridge.checkForUpdates()
                    }
                }
            }

        }
    }
}

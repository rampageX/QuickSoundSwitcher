import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick
import QtMultimedia
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
    Material.theme: Material.System
    title: qsTr("QuickSoundSwitcher - Settings")
    color: Material.theme === Material.Dark ? "#1C1C1C" : "#E3E3E3"

    property int rowHeight: 35

    Component.onCompleted: {
        updateKeepAliveAudioDevice()
        if (UserSettings.keepAlive) {
            startKeepAlive()
        }
    }

    MediaDevices {
        id: mediaDevices
        onAudioOutputsChanged: {
            root.updateKeepAliveAudioDevice()
        }
    }

    MediaPlayer {
        id: keepAlivePlayer
        source: "qrc:/sounds/empty.wav"
        audioOutput: AudioOutput {
            id: keepAliveAudioOutput
            volume: 0.01
            device: mediaDevices.defaultAudioOutput
        }
        loops: MediaPlayer.Infinite
    }

    function updateKeepAliveAudioDevice() {
        const device = mediaDevices.defaultAudioOutput
        keepAliveAudioOutput.device = device
        if (keepAlivePlayer.playbackState === MediaPlayer.PlayingState) {
            keepAlivePlayer.stop()
            keepAlivePlayer.play()
        }
    }

    function startKeepAlive() {
        if (keepAlivePlayer.playbackState !== MediaPlayer.PlayingState) {
            updateKeepAliveAudioDevice()
            console.log("pass")
            keepAlivePlayer.play()
        }
    }

    function stopKeepAlive() {
        if (keepAlivePlayer.playbackState === MediaPlayer.PlayingState) {
            keepAlivePlayer.stop()
        }
    }

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
                    if (checked) {
                        root.startKeepAlive()
                    } else {
                        root.stopKeepAlive()
                    }
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

            CheckBox {
                checked: SoundPanelBridge.getShortcutState()
                onClicked: SoundPanelBridge.setStartupShortcut(checked)
            }
        }
    }
}

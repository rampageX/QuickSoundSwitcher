pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.FluentWinUI3
import Odizinne.QuickSoundSwitcher

Rectangle {
    id: root

    property alias model: applicationsList.model
    property bool expanded: false
    property real contentOpacity: 0
    property string executableName: ""

    // Export the height needed when fully expanded
    property real expandedNeededHeight: {
        if (applicationsList.count < 2 || !model) {
            return 0
        }
        return applicationsList.contentHeight + 20
    }

    // Signals
    signal applicationVolumeChanged(string appId, int volume)
    signal applicationMuteChanged(string appId, bool muted)

    Layout.fillWidth: true
    Layout.preferredHeight: expanded ? expandedNeededHeight : 0
    Layout.leftMargin: -14
    Layout.rightMargin: -14
    color: Constants.footerColor

    Behavior on Layout.preferredHeight {
        NumberAnimation {
            duration: 150
            easing.type: Easing.OutQuad
        }
    }

    // Top border
    Rectangle {
        visible: root.expanded
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        color: Constants.footerBorderColor
        height: 1
        opacity: 0.1
    }

    // Bottom border
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
        id: applicationsList
        anchors.fill: parent
        anchors.topMargin: 10
        anchors.bottomMargin: 10
        anchors.leftMargin: 14
        anchors.rightMargin: 14
        clip: true
        interactive: false
        opacity: root.contentOpacity

        Behavior on opacity {
            NumberAnimation {
                duration: 200
                easing.type: Easing.OutQuad
            }
        }

        delegate: RowLayout {
            id: individualAppLayout
            width: applicationsList.width
            height: 40
            spacing: 0
            required property var model
            required property int index

            ToolButton {
                id: muteButton
                Layout.preferredWidth: 40
                Layout.preferredHeight: 40
                flat: !checked
                checkable: true
                highlighted: checked
                checked: individualAppLayout.model.isMuted
                ToolTip.text: individualAppLayout.model.name
                ToolTip.visible: hovered
                ToolTip.delay: 1000
                opacity: highlighted ? 0.3 : (enabled ? 1 : 0.5)
                icon.color: "transparent"
                icon.source: individualAppLayout.model.name == qsTr("System sounds") ? Constants.systemIcon : individualAppLayout.model.iconPath

                onClicked: {
                    let newMutedState = !individualAppLayout.model.isMuted
                    root.applicationMuteChanged(individualAppLayout.model.appId, newMutedState)
                }

                Component.onCompleted: {
                    palette.accent = palette.button
                }
            }

            ColumnLayout {
                spacing: -4

                Label {
                    visible: UserSettings.displayDevAppLabel
                    opacity: UserSettings.chatMixEnabled ? 0.3 : 0.5
                    elide: Text.ElideRight
                    Layout.preferredWidth: 200
                    Layout.leftMargin: 18
                    Layout.rightMargin: 25
                    text: {
                        let name = individualAppLayout.model.name
                        if (UserSettings.chatMixEnabled && AudioBridge.isCommApp(name)) {
                            name += " (Comm)"
                        }

                        if (name === "System sounds") {
                            return qsTr("System sounds")
                        }
                        return name
                    }
                }

                ProgressSlider {
                    id: volumeSlider
                    from: 0
                    to: 100
                    enabled: !UserSettings.chatMixEnabled && !muteButton.highlighted
                    opacity: enabled ? 1 : 0.5
                    audioLevel: {
                        return AudioBridge.getApplicationAudioLevel(individualAppLayout.model.appId)
                    }
                    Layout.fillWidth: true

                    // Break binding loop for individual sessions too
                    value: {
                        if (pressed) {
                            return value // Keep current value while dragging
                        }

                        if (!UserSettings.chatMixEnabled) {
                            return individualAppLayout.model.volume
                        }

                        let appName = individualAppLayout.model.name
                        let isCommApp = AudioBridge.isCommApp(appName)

                        return isCommApp ? 100 : UserSettings.chatMixValue
                    }

                    ToolTip {
                        parent: volumeSlider.handle
                        visible: volumeSlider.pressed && (UserSettings.volumeValueMode === 0)
                        text: Math.round(volumeSlider.value).toString()
                    }

                    onValueChanged: {
                        if (!UserSettings.chatMixEnabled && pressed) {
                            root.applicationVolumeChanged(individualAppLayout.model.appId, value)
                        }
                    }

                    onPressedChanged: {
                        if (!pressed && !UserSettings.chatMixEnabled) {
                            // Final update when releasing
                            root.applicationVolumeChanged(individualAppLayout.model.appId, value)
                        }

                        if (!UserSettings.showAudioLevel) return

                        if (pressed) {
                            AudioBridge.stopAudioLevelMonitoring()
                        } else {
                            AudioBridge.startAudioLevelMonitoring()
                        }
                    }
                }
            }

            Label {
                text: Math.round(volumeSlider.value).toString()
                Layout.rightMargin: 5
                font.pixelSize: 14
                opacity: UserSettings.chatMixEnabled ? 0.5 : 1
                visible: UserSettings.volumeValueMode === 1
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
}

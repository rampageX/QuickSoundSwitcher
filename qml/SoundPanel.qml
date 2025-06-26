import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.FluentWinUI3
import QtQuick.Window
import Odizinne.QuickSoundSwitcher

ApplicationWindow {
    id: panel
    width: 360
    height: mainLayout.implicitHeight
    visible: false
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    color: "transparent"
    property bool isAnimatingIn: false
    property bool isAnimatingOut: false
    property int margin: 12
    property int taskbarHeight: 52

    signal hideAnimationFinished()
    signal showAnimationFinished()
    signal hideAnimationStarted()


    Component.onCompleted: {
        if (UserSettings.panelMode === 2) {
            panel.height = mainLayout.implicitHeight + 30
        }
    }

    PropertyAnimation {
        id: showAnimation
        target: panel
        property: "y"
        duration: 210
        easing.type: Easing.OutCubic
        onFinished: {
            panel.isAnimatingIn = false
            mainLayout.opacity = 1
            panel.showAnimationFinished()
        }
    }

    PropertyAnimation {
        id: hideAnimation
        target: panel
        property: "y"
        duration: 210
        easing.type: Easing.InCubic
        onFinished: {
            panel.visible = false
            panel.isAnimatingOut = false
            panel.hideAnimationFinished()
        }
    }

    function showPanel() {
        if (isAnimatingIn || isAnimatingOut) {
            return
        }

        isAnimatingIn = true

        const screenWidth = Screen.width
        const screenHeight = Screen.height

        const panelX = screenWidth - width - margin
        const startY = screenHeight - taskbarHeight
        const targetY = screenHeight - height - margin - taskbarHeight

        const safeX = Math.min(Math.max(panelX, 0), screenWidth - width)
        const safeTargetY = Math.min(Math.max(targetY, 0), screenHeight - height - taskbarHeight)

        panel.x = safeX
        panel.y = startY

        panel.visible = true

        showAnimation.from = startY
        showAnimation.to = safeTargetY
        showAnimation.start()
    }

    function hidePanel() {
        if (isAnimatingIn || isAnimatingOut) {
            return
        }

        isAnimatingOut = true
        panel.hideAnimationStarted()

        const startY = panel.y
        const targetY = Screen.height - taskbarHeight

        hideAnimation.from = startY
        hideAnimation.to = targetY

        hideAnimation.start()
    }

    ListModel {
        id: playbackDeviceModel
    }

    ListModel {
        id: recordingDeviceModel
    }

    ListModel {
        id: appModel
    }

    Connections {
        target: SoundPanelBridge

        function onPlaybackDevicesChanged(devices) {
            playbackDeviceModel.clear()
            let defaultIndex = -1
            for (let i = 0; i < devices.length; i++) {
                playbackDeviceModel.append({
                                               id: devices[i].id,
                                               name: devices[i].name,
                                               shortName: devices[i].shortName,
                                               isDefault: devices[i].isDefault
                                           })
                if (devices[i].isDefault) {
                    defaultIndex = i
                }
            }
            if (defaultIndex !== -1) {
                outputDeviceComboBox.currentIndex = defaultIndex
            }
        }

        function onRecordingDevicesChanged(devices) {
            recordingDeviceModel.clear()
            let defaultIndex = -1
            for (let i = 0; i < devices.length; i++) {
                recordingDeviceModel.append({
                                                id: devices[i].id,
                                                name: devices[i].name,
                                                shortName: devices[i].shortName,
                                                isDefault: devices[i].isDefault
                                            })
                if (devices[i].isDefault) {
                    defaultIndex = i
                }
            }
            if (defaultIndex !== -1) {
                inputDeviceComboBox.currentIndex = defaultIndex
            }
        }

        function onApplicationsChanged(apps) {
            appModel.clear()
            for (let i = 0; i < apps.length; i++) {
                appModel.append(apps[i])
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "#242424"
        radius: 12
        Rectangle {
            anchors.fill: parent
            color: "transparent"
            radius: 12
            border.width: 1
            border.color: "#E3E3E3"
            opacity: 0.2
        }
    }

    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        anchors.margins: 15
        spacing: 5
        Behavior on opacity {
            NumberAnimation {
                duration: 300
                easing.type: Easing.OutQuad
            }
        }

        ColumnLayout {
            id: deviceLayout
            objectName: "gridLayout"
            spacing: 5
            visible: UserSettings.panelMode === 0 || UserSettings.panelMode === 2
            //Layout.bottomMargin: UserSettings.panelMode === 2 ? -10 : 0

            ComboBox {
                id: outputDeviceComboBox
                Layout.preferredHeight: 40
                Layout.columnSpan: 3
                Layout.fillWidth: true
                flat: true
                font.pixelSize: 15
                model: playbackDeviceModel
                textRole: "shortName"
                contentItem: Label {
                    text: outputDeviceComboBox.currentText
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignLeft
                    font.pixelSize: 15
                    leftPadding: 10
                    width: parent.width
                }
                onActivated: {
                    SoundPanelBridge.onPlaybackDeviceChanged(outputDeviceComboBox.currentText)
                    if (UserSettings.linkIO) {
                        const selectedText = outputDeviceComboBox.currentText

                        for (let i = 0; i < inputDeviceComboBox.count; ++i) {
                            if (inputDeviceComboBox.textAt(i) === selectedText) {
                                inputDeviceComboBox.currentIndex = i
                                break
                            }
                        }

                        SoundPanelBridge.onRecordingDeviceChanged(outputDeviceComboBox.currentText)
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                spacing: 0
                ToolButton {
                    id: outputeMuteRoundButton
                    Layout.preferredHeight: 40
                    Layout.preferredWidth: 40
                    flat: true
                    property string volumeIcon: {
                        if (SoundPanelBridge.playbackMuted || SoundPanelBridge.playbackVolume === 0) {
                            return "qrc:/icons/panel_volume_0.png"
                        } else if (SoundPanelBridge.playbackVolume <= 33) {
                            return "qrc:/icons/panel_volume_33.png"
                        } else if (SoundPanelBridge.playbackVolume <= 66) {
                            return "qrc:/icons/panel_volume_66.png"
                        } else {
                            return "qrc:/icons/panel_volume_100.png"
                        }
                    }

                    icon.source: volumeIcon
                    onClicked: {
                        SoundPanelBridge.onOutputMuteButtonClicked()
                    }
                }

                Slider {
                    id: outputSlider
                    value: pressed ? value : SoundPanelBridge.playbackVolume  // Only bind when not pressed
                    from: 0
                    to: 100
                    //stepSize: 1
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40

                    onValueChanged: {
                        if (pressed) {
                            SoundPanelBridge.onPlaybackVolumeChanged(value)
                        }
                    }

                    onPressedChanged: {
                        if (!pressed) {
                            // Sync final value when released
                            SoundPanelBridge.onPlaybackVolumeChanged(value)
                            SoundPanelBridge.onOutputSliderReleased()
                        }
                    }
                }
            }

            ComboBox {
                id: inputDeviceComboBox
                Layout.topMargin: -2
                Layout.preferredHeight: 40
                Layout.fillWidth: true
                Layout.columnSpan: 3
                flat: true
                font.pixelSize: 15
                model: recordingDeviceModel
                textRole: "shortName"
                contentItem: Label {
                    text: inputDeviceComboBox.currentText
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignLeft
                    font.pixelSize: 15
                    width: parent.width
                    leftPadding: 10
                }
                onActivated: {
                    SoundPanelBridge.onRecordingDeviceChanged(inputDeviceComboBox.currentText)

                    if (UserSettings.linkIO) {
                        const selectedText = inputDeviceComboBox.currentText

                        for (let i = 0; i < outputDeviceComboBox.count; ++i) {
                            if (outputDeviceComboBox.textAt(i) === selectedText) {
                                outputDeviceComboBox.currentIndex = i
                                break
                            }
                        }

                        SoundPanelBridge.onPlaybackDeviceChanged(inputDeviceComboBox.currentText)
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                spacing: 0
                ToolButton {
                    id: inputMuteRoundButton
                    Layout.preferredWidth: 40
                    Layout.preferredHeight: 40
                    flat: true
                    icon.source: SoundPanelBridge.recordingMuted ? "qrc:/icons/mic_muted.png" : "qrc:/icons/mic.png"
                    onClicked: {
                        SoundPanelBridge.onInputMuteButtonClicked()
                    }
                }

                Slider {
                    id: inputSlider
                    value: pressed ? value : SoundPanelBridge.recordingVolume  // Only bind when not pressed
                    from: 0
                    to: 100
                    //stepSize: 1
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40

                    onValueChanged: {
                        if (pressed) {
                            SoundPanelBridge.onRecordingVolumeChanged(value)
                        }
                    }

                    onPressedChanged: {
                        if (!pressed) {
                            // Sync final value when released
                            SoundPanelBridge.onRecordingVolumeChanged(value)
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.preferredHeight: 1
            Layout.fillWidth: true
            color: "#E3E3E3"
            opacity: 0.2
            visible: UserSettings.panelMode === 0
            Layout.rightMargin: -14
            Layout.leftMargin: -14
            Layout.topMargin: 5
            Layout.bottomMargin: 5
        }

        ColumnLayout {
            id: appLayout
            objectName: "gridLayout"
            spacing: 5
            visible: UserSettings.panelMode === 0 || UserSettings.panelMode === 1

            Repeater {
                id: appRepeater
                model: appModel
                onCountChanged: {
                    if (UserSettings.panelMode === 1) {
                        panel.height = mainLayout.implicitHeight + (45 * appRepeater.count) + 30
                    } else if (UserSettings.panelMode === 0) {
                        panel.height = mainLayout.implicitHeight + ((45 * appRepeater.count) - 5) + 30
                    } else {
                        panel.height = mainLayout.implicitHeight + 30
                    }
                }
                delegate: RowLayout {
                    id: applicationUnitLayout
                    Layout.preferredHeight: 40
                    Layout.fillWidth: true
                    spacing: 0
                    required property var model

                    ToolButton {
                        id: muteRoundButton
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        flat: !checked
                        checkable: true
                        highlighted: checked
                        checked: applicationUnitLayout.model.isMuted
                        ToolTip.text: applicationUnitLayout.model.name
                        ToolTip.visible: hovered
                        ToolTip.delay: 1000
                        opacity: highlighted ? 0.3 : 1
                        icon.source: applicationUnitLayout.model.name === "Windows system sounds" ? "qrc:/icons/system_light.png" : applicationUnitLayout.model.icon
                        icon.color: applicationUnitLayout.model.name === "Windows system sounds" ? undefined : "transparent"
                        onClicked: {
                            applicationUnitLayout.model.isMuted = !applicationUnitLayout.model.isMuted
                            SoundPanelBridge.onApplicationMuteButtonClicked(applicationUnitLayout.model.appID, applicationUnitLayout.model.isMuted)
                        }
                    }

                    Slider {
                        id: volumeSlider
                        from: 0
                        to: 100
                        enabled: !muteRoundButton.highlighted
                        value: applicationUnitLayout.model.volume
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        onValueChanged: {
                            SoundPanelBridge.onApplicationVolumeSliderValueChanged(applicationUnitLayout.model.appID, value)
                        }
                    }
                }
            }
        }

        Rectangle {
            color: "#1c1c1c"
            Layout.fillWidth: true
            Layout.fillHeight: true
            bottomLeftRadius: 12
            bottomRightRadius: 12
            Layout.preferredHeight: 50
            Layout.leftMargin: -14
            Layout.rightMargin: -14
            Layout.bottomMargin: -14
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10

                Item {
                    Layout.fillWidth: true
                }

                ToolButton {
                    icon.source: "qrc:/icons/settings.svg"
                    icon.width: 14
                    icon.height: 14
                    antialiasing: true
                }
            }
        }
    }
}


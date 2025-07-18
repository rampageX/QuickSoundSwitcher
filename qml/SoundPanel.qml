pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.FluentWinUI3
import QtQuick.Controls.impl
import QtQuick.Window
import Odizinne.QuickSoundSwitcher
import QtMultimedia

ApplicationWindow {
    id: panel
    width: 360
    height: {
        let newHeight = mainLayout.implicitHeight + 30 + 15
        if (mediaLayout.visible) {
            newHeight += mediaLayout.implicitHeight
        } else {
            newHeight -= 5
        }
        if (spacer.visible) {
            newHeight += spacer.height
        }
        newHeight += panel.maxDeviceListSpace
        return newHeight
    }

    SoundEffect {
        id: audioFeedback
        volume: 1
        source: "qrc:/sounds/windows-background.wav"
    }

    SystemTray {
        onTogglePanelRequested: {
            if (panel.visible) {
                panel.hidePanel()
            } else {
                panel.showPanel()
            }
        }
    }
    MouseArea {
        height: panel.maxDeviceListSpace - panel.currentUsedListSpace
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: UserSettings.panelPosition === 0 ? parent.bottom : undefined
        anchors.top: UserSettings.panelPosition === 0 ? undefined : parent.top
        onClicked: panel.hidePanel()
    }

    Shortcut {
        sequence: StandardKey.Cancel
        onActivated: {
            if (!panel.isAnimatingIn && !panel.isAnimatingOut && panel.visible) {
                panel.hidePanel()
            }
        }
    }

    visible: false
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    color: "#00000000"
    property bool isAnimatingIn: false
    property bool isAnimatingOut: false
    property string taskbarPos: SoundPanelBridge.taskbarPosition

    property real maxDeviceListSpace: {
        let outputSpace = outputDevicesRect.expandedNeededHeight || 0
        let inputSpace = inputDevicesRect.expandedNeededHeight || 0
        return outputSpace + inputSpace
    }

    property real currentUsedListSpace: {
        let usedSpace = 0
        if (outputDevicesRect.expanded) {
            usedSpace += outputDevicesRect.expandedNeededHeight || 0
        }
        if (inputDevicesRect.expanded) {
            usedSpace += inputDevicesRect.expandedNeededHeight || 0
        }
        return usedSpace
    }

    property real listCompensationOffset: maxDeviceListSpace - currentUsedListSpace

    signal hideAnimationFinished()
    signal showAnimationFinished()
    signal hideAnimationStarted()

    ChatMixNotification {}

    onVisibleChanged: {
        if (!visible) {
            outputDevicesRect.expanded = false
            inputDevicesRect.expanded = false
            if (UserSettings.opacityAnimations) {
                mediaLayout.opacity = 0
                mainLayout.opacity = 0
            }

            if (UserSettings.showAudioLevel) {
                SoundPanelBridge.stopAudioLevelMonitoring()
            }
        } else {
            if (UserSettings.showAudioLevel) {
                SoundPanelBridge.startAudioLevelMonitoring()
            }
        }
    }

    Timer {
        id: contentOpacityTimer
        interval: 160
        repeat: false
        onTriggered: mainLayout.opacity = 1
    }

    Timer {
        id: flyoutOpacityTimer
        interval: 160
        repeat: false
        onTriggered: mediaLayout.opacity = 1
    }

    onHeightChanged: {
        if (isAnimatingIn && !isAnimatingOut) {
            positionPanelAtTarget()
        }
    }

    onShowAnimationFinished: {
        SoundPanelBridge.startMediaMonitoring()
    }

    onHideAnimationStarted: {
        SoundPanelBridge.stopMediaMonitoring()
    }

    PropertyAnimation {
        id: showAnimation
        target: contentTransform
        duration: 180
        easing.type: Easing.OutCubic
        onStarted: {
            contentOpacityTimer.start()
            flyoutOpacityTimer.start()
        }
        onFinished: {
            panel.isAnimatingIn = false
            panel.showAnimationFinished()
        }
    }

    PropertyAnimation {
        id: hideAnimation
        target: contentTransform
        duration: 180
        easing.type: Easing.InCubic
        onFinished: {
            panel.visible = false
            panel.isAnimatingOut = false
            panel.hideAnimationFinished()
        }
    }

    Translate {
        id: contentTransform
        property real x: 0
        property real y: 0
    }

    Translate {
        id: listCompensationTransform
        x: 0
        y: (panel.isAnimatingIn || panel.isAnimatingOut) ? 0 : -panel.listCompensationOffset

        Behavior on y {
            enabled: !panel.isAnimatingIn && !panel.isAnimatingOut
            NumberAnimation {
                duration: 150
                easing.type: Easing.OutQuad
            }
        }
    }

    function showPanel() {
        if (isAnimatingIn || isAnimatingOut) {
            return
        }

        isAnimatingIn = true
        Constants.darkMode = SoundPanelBridge.getDarkMode()
        panel.taskbarPos = SoundPanelBridge.taskbarPosition
        panel.visible = true

        positionPanelAtTarget()
        setInitialTransform()
        // Start animation immediately since models are always ready

        Qt.callLater(function() {
            Qt.callLater(function() {
                let newHeight = mainLayout.implicitHeight + 30 + 15

                if (mediaLayout.visible) {
                    newHeight += mediaLayout.implicitHeight
                }

                if (spacer.visible) {
                    newHeight += spacer.height
                }

                newHeight += panel.maxDeviceListSpace

                panel.height = newHeight
                Qt.callLater(panel.startAnimation)
            })
        })
        //Qt.callLater(panel.startAnimation)
    }

    function positionPanelAtTarget() {
        const screenWidth = Qt.application.screens[0].width
        const screenHeight = Qt.application.screens[0].height

        switch (panel.taskbarPos) {
        case "top":
            panel.x = screenWidth - width - UserSettings.xAxisMargin
            panel.y = UserSettings.yAxisMargin + UserSettings.taskbarOffset
            break
        case "bottom":
            panel.x = screenWidth - width - UserSettings.xAxisMargin
            panel.y = screenHeight - height - UserSettings.yAxisMargin - UserSettings.taskbarOffset
            break
        case "left":
            panel.x = UserSettings.taskbarOffset + UserSettings.xAxisMargin
            panel.y = screenHeight - height - UserSettings.yAxisMargin
            break
        case "right":
            panel.x = screenWidth - width - UserSettings.xAxisMargin - UserSettings.taskbarOffset
            panel.y = screenHeight - height - UserSettings.yAxisMargin
            break
        default:
            panel.x = screenWidth - width - UserSettings.xAxisMargin
            panel.y = screenHeight - height - UserSettings.yAxisMargin - UserSettings.taskbarOffset
            break
        }
    }

    function setInitialTransform() {
        switch (panel.taskbarPos) {
        case "top":
            contentTransform.y = -height
            contentTransform.x = 0
            break
        case "bottom":
            contentTransform.y = height
            contentTransform.x = 0
            break
        case "left":
            contentTransform.x = -width
            contentTransform.y = 0
            break
        case "right":
            contentTransform.x = width
            contentTransform.y = 0
            break
        default:
            contentTransform.y = height
            contentTransform.x = 0
            break
        }
    }

    function startAnimation() {
        if (!isAnimatingIn) return

        showAnimation.properties = panel.taskbarPos === "left" || panel.taskbarPos === "right" ? "x" : "y"
        showAnimation.from = panel.taskbarPos === "left" || panel.taskbarPos === "right" ? contentTransform.x : contentTransform.y
        showAnimation.to = 0
        showAnimation.start()
    }

    function hidePanel() {
        if (isAnimatingIn || isAnimatingOut) {
            return
        }

        isAnimatingOut = true
        panel.hideAnimationStarted()

        switch (panel.taskbarPos) {
        case "top":
            hideAnimation.properties = "y"
            hideAnimation.from = contentTransform.y
            hideAnimation.to = -height
            break
        case "bottom":
            hideAnimation.properties = "y"
            hideAnimation.from = contentTransform.y
            hideAnimation.to = height
            break
        case "left":
            hideAnimation.properties = "x"
            hideAnimation.from = contentTransform.x
            hideAnimation.to = -width
            break
        case "right":
            hideAnimation.properties = "x"
            hideAnimation.from = contentTransform.x
            hideAnimation.to = width
            break
        default:
            hideAnimation.properties = "y"
            hideAnimation.from = contentTransform.y
            hideAnimation.to = height
            break
        }

        hideAnimation.start()
    }

    Connections {
        target: SoundPanelBridge
        function onChatMixEnabledChanged(enabled) {
            UserSettings.chatMixEnabled = enabled

            if (enabled) {
                AudioBridge.applyChatMixToApplications(UserSettings.chatMixValue)
            } else {
                AudioBridge.restoreOriginalVolumes()
            }
        }
    }

    KeepAlive {}

    SettingsWindow {
        id: settingsWindow
    }

    Item {
        id: cont
        anchors.bottom: UserSettings.panelPosition === 0 ? undefined : parent.bottom
        anchors.top: UserSettings.panelPosition === 0 ? parent.top : undefined
        anchors.right: parent.right
        anchors.left: parent.left
        transform: Translate {
            x: contentTransform.x
            y: contentTransform.y
        }

        height: {
            let newHeight = mainLayout.implicitHeight + 30 + 15
            if (mediaLayout.visible) {
                newHeight += mediaLayout.implicitHeight
            } else {
                newHeight -= 5
            }
            if (spacer.visible) {
                newHeight += spacer.height
            }
            return newHeight
        }

        Rectangle {
            anchors.fill: mainLayout
            anchors.margins: -15
            color: Constants.panelColor
            radius: 12
            Rectangle {
                anchors.fill: parent
                color: "#00000000"
                radius: 12
                border.width: 1
                border.color: "#E3E3E3"
                opacity: 0.2
            }
        }

        Rectangle {
            anchors.fill: mediaLayout
            anchors.margins: -15
            color: Constants.panelColor
            visible: mediaLayout.visible
            radius: 12
            Rectangle {
                anchors.fill: parent
                color: "#00000000"
                radius: 12
                border.width: 1
                border.color: "#E3E3E3"
                opacity: 0.2
            }
        }

        MediaFlyoutContent {
            id: mediaLayout
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 15
            opacity: 0
            visible: UserSettings.mediaMode === 0 && (SoundPanelBridge.mediaTitle !== "")

            Behavior on opacity {
                NumberAnimation {
                    duration: 300
                    easing.type: Easing.OutQuad
                }
            }
        }

        Item {
            id: spacer
            anchors.top: mediaLayout.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 30
            visible: mediaLayout.visible
        }

        ColumnLayout {
            id: mainLayout
            anchors.top: mediaLayout.visible ? spacer.bottom : parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 15
            spacing: 10
            opacity: 0

            Behavior on opacity {
                NumberAnimation {
                    duration: 300
                    easing.type: Easing.OutQuad
                }
            }

            ColumnLayout {
                id: deviceLayout
                spacing: 5
                visible: UserSettings.panelMode === 0 || UserSettings.panelMode === 2

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    spacing: 0
                    ToolButton {
                        Layout.preferredHeight: 40
                        Layout.preferredWidth: 40
                        flat: true
                        property string volumeIcon: {
                            if (AudioBridge.outputMuted || AudioBridge.outputVolume === 0) {
                                return "qrc:/icons/panel_volume_0.png"
                            } else if (AudioBridge.outputVolume <= 33) {
                                return "qrc:/icons/panel_volume_33.png"
                            } else if (AudioBridge.outputVolume <= 66) {
                                return "qrc:/icons/panel_volume_66.png"
                            } else {
                                return "qrc:/icons/panel_volume_100.png"
                            }
                        }

                        icon.source: volumeIcon
                        onClicked: {
                            AudioBridge.setOutputMute(!AudioBridge.outputMuted)
                        }
                    }

                    ColumnLayout {
                        spacing: -4
                        Label {
                            visible: UserSettings.displayDevAppLabel
                            opacity: 0.5
                            elide: Text.ElideRight
                            Layout.preferredWidth: outputSlider.implicitWidth - 30
                            Layout.leftMargin: 18
                            Layout.rightMargin: 25
                            text: {
                                if (!AudioBridge.isReady) return ""

                                let defaultIndex = AudioBridge.outputDevices.currentDefaultIndex
                                if (defaultIndex >= 0) {
                                    if (UserSettings.deviceShortName) {
                                        return AudioBridge.outputDevices.getDeviceShortName(defaultIndex)
                                    } else {
                                        return AudioBridge.outputDevices.getDeviceName(defaultIndex)
                                    }
                                }
                                return ""
                            }
                        }

                        ProgressSlider {
                            id: outputSlider
                            value: pressed ? value : AudioBridge.outputVolume
                            from: 0
                            to: 100
                            Layout.fillWidth: true
                            //audioLevel: SoundPanelBridge.playbackAudioLevel

                            ToolTip {
                                parent: outputSlider.handle
                                visible: outputSlider.pressed && (UserSettings.volumeValueMode === 0)
                                text: Math.round(outputSlider.value).toString()
                            }

                            onValueChanged: {
                                if (pressed) {
                                    AudioBridge.setOutputVolume(value)
                                }
                            }

                            onPressedChanged: {
                                if (!pressed) {
                                    AudioBridge.setOutputVolume(value)
                                    audioFeedback.play()
                                }
                            }
                        }
                    }

                    Label {
                        text: Math.round(outputSlider.value).toString()
                        Layout.rightMargin: 5
                        font.pixelSize: 14
                        visible: UserSettings.volumeValueMode === 1
                    }

                    ToolButton {
                        icon.source: "qrc:/icons/arrow.svg"
                        rotation: outputDevicesRect.expanded ? 90 : 0
                        visible: AudioBridge.isReady && AudioBridge.outputDevices.rowCount() > 1
                        Layout.preferredHeight: 35
                        Layout.preferredWidth: 35
                        Behavior on rotation {
                            NumberAnimation {
                                duration: 150
                                easing.type: Easing.Linear
                            }
                        }

                        onClicked: {
                            outputDevicesRect.expanded = !outputDevicesRect.expanded
                        }
                    }
                }

                DevicesListView {
                    id: outputDevicesRect
                    model: AudioBridge.outputDevices
                    onDeviceClicked: function(name, shortName, index) {
                        AudioBridge.setOutputDevice(index)

                        if (UserSettings.linkIO) {
                            // Try to find matching input device using shortName
                            let inputModel = AudioBridge.inputDevices
                            for (let i = 0; i < inputModel.rowCount(); ++i) {
                                let inputDeviceShortName = inputModel.getDeviceShortName(i)
                                if (inputDeviceShortName === shortName) {
                                    AudioBridge.setInputDevice(i)
                                    break
                                }
                            }
                        }
                        if (UserSettings.closeDeviceListOnClick) {
                            expanded = false
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    spacing: 0
                    ToolButton {
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        flat: true
                        icon.source: AudioBridge.inputMuted ? "qrc:/icons/mic_muted.png" : "qrc:/icons/mic.png"
                        onClicked: {
                            AudioBridge.setInputMute(!AudioBridge.inputMuted)
                        }
                    }

                    ColumnLayout {
                        spacing: -4
                        Label {
                            visible: UserSettings.displayDevAppLabel
                            opacity: 0.5
                            elide: Text.ElideRight
                            Layout.preferredWidth: inputSlider.implicitWidth - 30
                            Layout.leftMargin: 18
                            Layout.rightMargin: 25
                            text: {
                                if (!AudioBridge.isReady) return ""

                                let defaultIndex = AudioBridge.inputDevices.currentDefaultIndex
                                if (defaultIndex >= 0) {
                                    if (UserSettings.deviceShortName) {
                                        return AudioBridge.inputDevices.getDeviceShortName(defaultIndex)
                                    } else {
                                        return AudioBridge.inputDevices.getDeviceName(defaultIndex)
                                    }
                                }
                                return ""
                            }
                        }

                        ProgressSlider {
                            id: inputSlider
                            value: pressed ? value : AudioBridge.inputVolume
                            from: 0
                            to: 100
                            //audioLevel: SoundPanelBridge.recordingAudioLevel
                            Layout.fillWidth: true

                            ToolTip {
                                parent: inputSlider.handle
                                visible: inputSlider.pressed && (UserSettings.volumeValueMode === 0)
                                text: Math.round(inputSlider.value).toString()
                            }

                            onValueChanged: {
                                if (pressed) {
                                    AudioBridge.setInputVolume(value)
                                }
                            }

                            onPressedChanged: {
                                if (!pressed) {
                                    AudioBridge.setInputVolume(value)
                                }
                            }
                        }
                    }

                    Label {
                        text: Math.round(inputSlider.value).toString()
                        Layout.rightMargin: 5
                        font.pixelSize: 14
                        visible: UserSettings.volumeValueMode === 1
                    }

                    ToolButton {
                        icon.source: "qrc:/icons/arrow.svg"
                        rotation: inputDevicesRect.expanded ? 90 : 0
                        Layout.preferredHeight: 35
                        Layout.preferredWidth: 35
                        visible: AudioBridge.isReady && AudioBridge.inputDevices.rowCount() > 1
                        Behavior on rotation {
                            NumberAnimation {
                                duration: 150
                                easing.type: Easing.Linear
                            }
                        }

                        onClicked: {
                            inputDevicesRect.expanded = !inputDevicesRect.expanded
                        }
                    }
                }

                DevicesListView {
                    id: inputDevicesRect
                    model: AudioBridge.inputDevices
                    onDeviceClicked: function(name, shortName, index) {
                        AudioBridge.setInputDevice(index)
                        if (UserSettings.linkIO) {
                            // Try to find matching output device using shortName
                            let outputModel = AudioBridge.outputDevices
                            for (let i = 0; i < outputModel.rowCount(); ++i) {
                                let outputDeviceShortName = outputModel.getDeviceShortName(i)
                                if (outputDeviceShortName === shortName) {
                                    AudioBridge.setOutputDevice(i)
                                    break
                                }
                            }
                        }
                        if (UserSettings.closeDeviceListOnClick) {
                            expanded = false
                        }
                    }
                }
            }

            Rectangle {
                Layout.preferredHeight: 1
                Layout.fillWidth: true
                color: Constants.separatorColor
                opacity: 0.15
                visible: UserSettings.panelMode === 0 && AudioBridge.isReady && AudioBridge.applications.rowCount() > 0
                Layout.rightMargin: -14
                Layout.leftMargin: -14
            }

            ColumnLayout {
                id: appLayout
                spacing: 5
                visible: UserSettings.panelMode < 2 && AudioBridge.isReady && AudioBridge.applications.rowCount() > 0

                Repeater {
                    model: AudioBridge.applications
                    delegate: RowLayout {
                        id: applicationUnitLayout
                        Layout.preferredHeight: 40
                        Layout.fillWidth: true
                        spacing: 0
                        required property var model
                        required property int index

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
                            opacity: highlighted ? 0.3 : (enabled ? 1 : 0.5)
                            icon.color: "transparent"
                            icon.source: applicationUnitLayout.model.name == qsTr("System sounds") ? Constants.systemIcon : applicationUnitLayout.model.iconPath

                            onClicked: {
                                let newMutedState = !applicationUnitLayout.model.isMuted
                                AudioBridge.setApplicationMute(applicationUnitLayout.model.appId, newMutedState)
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
                                    let name = applicationUnitLayout.model.name
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
                                enabled: !UserSettings.chatMixEnabled && !muteRoundButton.highlighted
                                opacity: enabled ? 1 : 0.5
                                audioLevel: applicationUnitLayout.model.audioLevel
                                Layout.fillWidth: true

                                value: {
                                    if (!UserSettings.chatMixEnabled) {
                                        return applicationUnitLayout.model.volume
                                    }

                                    let appName = applicationUnitLayout.model.name
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
                                        AudioBridge.setApplicationVolume(applicationUnitLayout.model.appId, value)
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
            }

            Rectangle {
                visible: UserSettings.activateChatmix
                Layout.preferredHeight: 1
                Layout.fillWidth: true
                color: Constants.separatorColor
                opacity: 0.15
                Layout.rightMargin: -14
                Layout.leftMargin: -14
            }

            ColumnLayout {
                visible: UserSettings.activateChatmix
                id: chatMixLayout
                spacing: 5

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    spacing: 0

                    ToolButton {
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        icon.width: 14
                        icon.height: 14
                        icon.source: "qrc:/icons/headset.svg"
                        icon.color: palette.text
                        checkable: true
                        checked: !UserSettings.chatMixEnabled
                        opacity: checked ? 0.3 : 1

                        Component.onCompleted: {
                            palette.accent = palette.button
                        }

                        onClicked: {
                            UserSettings.chatMixEnabled = !checked

                            if (!checked) {
                                console.log("apply")
                                AudioBridge.applyChatMixToApplications(UserSettings.chatMixValue)
                            } else {
                                console.log("restore")
                                AudioBridge.restoreOriginalVolumes()
                            }
                        }
                    }

                    ColumnLayout {
                        spacing: -4

                        Label {
                            visible: UserSettings.displayDevAppLabel
                            opacity: 0.5
                            text: qsTr("ChatMix")
                            Layout.leftMargin: 18
                            Layout.rightMargin: 25
                        }

                        Slider {
                            id: chatMixSlider
                            value: UserSettings.chatMixValue
                            from: 0
                            to: 100
                            Layout.fillWidth: true
                            enabled: UserSettings.chatMixEnabled

                            onValueChanged: {
                                UserSettings.chatMixValue = value
                            }

                            onPressedChanged: {
                                if (pressed) return
                                if (UserSettings.chatMixEnabled) {
                                    AudioBridge.applyChatMixToApplications(Math.round(value))
                                }
                            }
                        }
                    }

                    IconImage {
                        Layout.preferredWidth: 35
                        Layout.preferredHeight: 35
                        sourceSize.width: 14
                        sourceSize.height: 14
                        color: palette.text
                        source: "qrc:/icons/music.svg"
                        enabled: UserSettings.chatMixEnabled
                    }
                }
            }

            Rectangle {
                color: Constants.footerColor
                Layout.fillWidth: true
                Layout.fillHeight: false
                bottomLeftRadius: 12
                bottomRightRadius: 12
                Layout.preferredHeight: 50
                Layout.leftMargin: -14
                Layout.rightMargin: -14
                Layout.bottomMargin: -14

                Rectangle {
                    height: 1
                    color: Constants.footerBorderColor
                    opacity: 0.15
                    visible: UserSettings.panelMode === 0
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.left: parent.left
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    spacing: 10

                    Image {
                        Layout.preferredWidth: 32
                        Layout.preferredHeight: 32
                        Layout.alignment: Qt.AlignVCenter
                        source: SoundPanelBridge.mediaArt || ""
                        fillMode: Image.PreserveAspectCrop
                        visible: SoundPanelBridge.mediaArt !== "" && UserSettings.mediaMode === 1
                    }

                    ColumnLayout {
                        spacing: 2
                        Layout.fillWidth: true
                        Item {
                            Layout.fillHeight: true
                            Layout.fillWidth: true
                        }

                        Label {
                            text: SoundPanelBridge.mediaTitle || ""
                            font.pixelSize: 14
                            font.bold: true
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                            visible: UserSettings.mediaMode === 1
                        }

                        Label {
                            text: SoundPanelBridge.mediaArtist || ""
                            font.pixelSize: 12
                            opacity: 0.7
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                            visible: UserSettings.mediaMode === 1
                        }

                        Item {
                            Layout.fillHeight: true
                        }
                    }

                    ToolButton {
                        icon.source: "qrc:/icons/settings.svg"
                        icon.width: 14
                        icon.height: 14
                        antialiasing: true
                        onClicked: {
                            settingsWindow.show()
                            panel.hidePanel()
                        }
                    }
                }
            }
        }
    }
}

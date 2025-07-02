pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.FluentWinUI3
import QtQuick.Controls.impl
import QtQuick.Window
import Odizinne.QuickSoundSwitcher

ApplicationWindow {
    id: panel
    width: 360
    height: 200
    visible: false
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    color: "transparent"
    property bool isAnimatingIn: false
    property bool isAnimatingOut: false
    property int margin: UserSettings.panelMargin
    property int taskbarHeight: UserSettings.taskbarOffset
    property bool dataLoaded: false
    property string taskbarPos: SoundPanelBridge.taskbarPosition

    signal hideAnimationFinished()
    signal showAnimationFinished()
    signal hideAnimationStarted()

    onVisibleChanged: {
        if (!visible) {
            outputDevicesRect.expanded = false
            inputDevicesRect.expanded = false
            mainLayout.opacity = 0
            mediaLayout.opacity = 0
            outputDevicesRect.contentOpacity = 0
            inputDevicesRect.contentOpacity = 0
            contentOpacityTimer.stop()
            outputListOpacityTimer.stop()
            inputListOpacityTimer.stop()
        }
    }

    // Simple ListView height animations
    PropertyAnimation {
        id: outputExpandAnimation
        target: outputDevicesRect
        property: "Layout.preferredHeight"
        duration: 150
        easing.type: Easing.OutQuad
    }

    PropertyAnimation {
        id: inputExpandAnimation
        target: inputDevicesRect
        property: "Layout.preferredHeight"
        duration: 150
        easing.type: Easing.OutQuad
    }

    Timer {
        id: contentOpacityTimer
        interval: 200
        repeat: false
        onTriggered: mainLayout.opacity = 1
    }

    Timer {
        id: flyoutOpacityTimer
        interval: 200
        repeat: false
        onTriggered: mediaLayout.opacity = 1
    }

    Timer {
        id: outputListOpacityTimer
        interval: 112
        repeat: false
        onTriggered: outputDevicesRect.contentOpacity = 1
    }

    Timer {
        id: inputListOpacityTimer
        interval: 112
        repeat: false
        onTriggered: inputDevicesRect.contentOpacity = 1
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
        duration: 210
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
        duration: 210
        easing.type: Easing.InCubic
        onFinished: {
            panel.visible = false
            panel.isAnimatingOut = false
            panel.dataLoaded = false
            panel.hideAnimationFinished()
        }
        onStarted: {
            mainLayout.opacity = 0
            mediaLayout.opacity = 0
            outputDevicesRect.contentOpacity = 0
            inputDevicesRect.contentOpacity = 0
        }
    }

    Translate {
        id: contentTransform
        property real x: 0
        property real y: 0
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
    }

    function positionPanelAtTarget() {
        const screenWidth = Qt.application.screens[0].width
        const screenHeight = Qt.application.screens[0].height

        switch (panel.taskbarPos) {
        case "top":
            panel.x = screenWidth - width - margin
            panel.y = margin + taskbarHeight
            break
        case "bottom":
            panel.x = screenWidth - width - margin
            panel.y = screenHeight - height - margin - taskbarHeight
            break
        case "left":
            panel.x = taskbarHeight
            panel.y = screenHeight - height - margin
            break
        case "right":
            panel.x = screenWidth - width - taskbarHeight
            panel.y = screenHeight - height - margin
            break
        default:
            panel.x = screenWidth - width - margin
            panel.y = screenHeight - height - margin - taskbarHeight
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
            for (let i = 0; i < devices.length; i++) {
                playbackDeviceModel.append({
                                               id: devices[i].id,
                                               name: devices[i].name,
                                               shortName: devices[i].shortName,
                                               isDefault: devices[i].isDefault
                                           })
            }
            panel.updatePanelHeight()
        }

        function onRecordingDevicesChanged(devices) {
            recordingDeviceModel.clear()
            for (let i = 0; i < devices.length; i++) {
                recordingDeviceModel.append({
                                                id: devices[i].id,
                                                name: devices[i].name,
                                                shortName: devices[i].shortName,
                                                isDefault: devices[i].isDefault
                                            })
            }
            panel.updatePanelHeight()
        }

        function onApplicationsChanged(apps) {
            appModel.clear()
            for (let i = 0; i < apps.length; i++) {
                appModel.append(apps[i])
            }
            panel.updatePanelHeight()
        }

        function onDataInitializationComplete() {
            panel.dataLoaded = true
            Qt.callLater(function() {
                Qt.callLater(function() {
                    const newHeight = mediaLayout.implicitHeight + spacer.height + mainLayout.implicitHeight + 30 + 15
                    panel.height = newHeight
                    Qt.callLater(panel.startAnimation)
                })
            })
        }
    }

    function updatePanelHeight() {
        Qt.callLater(function() {
            Qt.callLater(function() {
                const newHeight = mediaLayout.implicitHeight + spacer.height + mainLayout.implicitHeight + 30 + 15

                if (panel.visible && !panel.isAnimatingIn && !panel.isAnimatingOut) {
                    panel.height = newHeight
                    positionPanelAtTarget()
                } else {
                    panel.height = newHeight
                }
            })
        })
    }

    KeepAlive {}

    SettingsWindow {
        id: settingsWindow
    }

    Item {
        anchors.fill: parent
        transform: Translate {
            x: contentTransform.x
            y: contentTransform.y
        }

        Rectangle {
            anchors.fill: mainLayout
            anchors.margins: -15
            color: Constants.panelColor
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

        Rectangle {
            anchors.fill: mediaLayout
            anchors.margins: -15
            color: Constants.panelColor
            visible: mediaLayout.visible
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

        MediaFlyoutContent {
            id: mediaLayout
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 15
            onImplicitHeightChanged: {
                if (panel.visible) {
                    panel.updatePanelHeight()
                }
            }
        }

        Item {
            id: spacer
            anchors.top: mediaLayout.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 30

            onImplicitHeightChanged: {
                if (panel.visible) {
                    panel.updatePanelHeight()
                }
            }
        }

        ColumnLayout {
            id: mainLayout
            anchors.top: spacer.bottom
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 15
            spacing: 10
            opacity: 0

            onImplicitHeightChanged: {
                if (panel.visible) {
                    panel.updatePanelHeight()
                }
            }

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
                                for (let i = 0; i < playbackDeviceModel.count; i++) {
                                    if (playbackDeviceModel.get(i).isDefault) {
                                        return UserSettings.deviceShortName ?
                                            playbackDeviceModel.get(i).shortName :
                                            playbackDeviceModel.get(i).name
                                    }
                                }
                                return ""
                            }
                        }

                        Slider {
                            id: outputSlider
                            value: pressed ? value : SoundPanelBridge.playbackVolume
                            from: 0
                            to: 100
                            Layout.fillWidth: true

                            ToolTip {
                                parent: outputSlider.handle
                                visible: outputSlider.pressed && (UserSettings.volumeValueMode === 0)
                                text: Math.round(outputSlider.value).toString()
                            }

                            onValueChanged: {
                                if (pressed) {
                                    SoundPanelBridge.onPlaybackVolumeChanged(value)
                                }
                            }

                            onPressedChanged: {
                                if (!pressed) {
                                    SoundPanelBridge.onPlaybackVolumeChanged(value)
                                    SoundPanelBridge.onOutputSliderReleased()
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
                        visible: playbackDeviceModel.count > 1
                        Layout.preferredHeight: 35
                        Layout.preferredWidth: 35
                        Behavior on rotation {
                            NumberAnimation {
                                duration: 150
                                easing.type: Easing.OutQuad
                            }
                        }

                        onClicked: {
                            outputDevicesRect.expanded = !outputDevicesRect.expanded
                        }
                    }
                }

                DevicesListView {
                    id: outputDevicesRect
                    model: playbackDeviceModel
                    onDeviceClicked: function(name, shortName, index) {
                        SoundPanelBridge.onPlaybackDeviceChanged(name)

                        if (UserSettings.linkIO) {
                            const selectedDeviceName = shortName
                            let linkedInputIndex = -1
                            for (let i = 0; i < recordingDeviceModel.count; i++) {
                                const inputName = recordingDeviceModel.get(i).shortName
                                if (inputName === selectedDeviceName) {
                                    linkedInputIndex = i
                                    break
                                }
                            }
                            if (linkedInputIndex !== -1) {
                                for (let i = 0; i < recordingDeviceModel.count; i++) {
                                    recordingDeviceModel.setProperty(i, "isDefault", i === linkedInputIndex)
                                }
                                SoundPanelBridge.onRecordingDeviceChanged(recordingDeviceModel.get(linkedInputIndex).name)
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
                        icon.source: SoundPanelBridge.recordingMuted ? "qrc:/icons/mic_muted.png" : "qrc:/icons/mic.png"
                        onClicked: {
                            SoundPanelBridge.onInputMuteButtonClicked()
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
                                for (let i = 0; i < recordingDeviceModel.count; i++) {
                                    if (recordingDeviceModel.get(i).isDefault) {
                                        return UserSettings.deviceShortName ?
                                            recordingDeviceModel.get(i).shortName :
                                            recordingDeviceModel.get(i).name
                                    }
                                }
                                return ""
                            }
                        }

                        Slider {
                            id: inputSlider
                            value: pressed ? value : SoundPanelBridge.recordingVolume
                            from: 0
                            to: 100
                            Layout.fillWidth: true

                            ToolTip {
                                parent: inputSlider.handle
                                visible: inputSlider.pressed && (UserSettings.volumeValueMode === 0)
                                text: Math.round(inputSlider.value).toString()
                            }

                            onValueChanged: {
                                if (pressed) {
                                    SoundPanelBridge.onRecordingVolumeChanged(value)
                                }
                            }

                            onPressedChanged: {
                                if (!pressed) {
                                    SoundPanelBridge.onRecordingVolumeChanged(value)
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
                        rotation: outputDevicesRect.expanded ? 90 : 0
                        Layout.preferredHeight: 35
                        Layout.preferredWidth: 35
                        visible: recordingDeviceModel.count > 1
                        Behavior on rotation {
                            NumberAnimation {
                                duration: 150
                                easing.type: Easing.OutQuad
                            }
                        }

                        onClicked: {
                            inputDevicesRect.expanded = !inputDevicesRect.expanded
                        }
                    }
                }

                DevicesListView {
                    id: inputDevicesRect
                    model: recordingDeviceModel
                    onDeviceClicked: function(name, shortName, index) {
                        SoundPanelBridge.onRecordingDeviceChanged(name)
                        if (UserSettings.linkIO) {
                            const selectedDeviceName = shortName
                            let linkedOutputIndex = -1
                            for (let i = 0; i < playbackDeviceModel.count; i++) {
                                const outputName = playbackDeviceModel.get(i).shortName
                                if (outputName === selectedDeviceName) {
                                    linkedOutputIndex = i
                                    break
                                }
                            }
                            if (linkedOutputIndex !== -1) {
                                for (let i = 0; i < playbackDeviceModel.count; i++) {
                                    playbackDeviceModel.setProperty(i, "isDefault", i === linkedOutputIndex)
                                }
                                SoundPanelBridge.onPlaybackDeviceChanged(playbackDeviceModel.get(linkedOutputIndex).name)
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
                visible: UserSettings.panelMode === 0
                Layout.rightMargin: -14
                Layout.leftMargin: -14
            }

            ColumnLayout {
                id: appLayout
                spacing: 5
                visible: UserSettings.panelMode === 0 || UserSettings.panelMode === 1

                Repeater {
                    id: appRepeater
                    model: appModel
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
                            enabled: !UserSettings.chatMixEnabled
                            ToolTip.text: applicationUnitLayout.model.name
                            ToolTip.visible: hovered
                            ToolTip.delay: 1000
                            opacity: highlighted ? 0.3 : (enabled ? 1 : 0.5)

                            icon.source: {
                                if (applicationUnitLayout.model.name === "System sounds") {
                                    return "qrc:/icons/system_light.png"
                                }
                                return applicationUnitLayout.model.icon || ""
                            }

                            icon.color: applicationUnitLayout.model.name === "System sounds" ? (Constants.darkMode ? "white" : "black") : "transparent"

                            onClicked: {
                                let newMutedState = !applicationUnitLayout.model.isMuted
                                appModel.setProperty(applicationUnitLayout.index, "isMuted", newMutedState)
                                SoundPanelBridge.onApplicationMuteButtonClicked(applicationUnitLayout.model.appID, newMutedState)
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
                                    if (UserSettings.chatMixEnabled && SoundPanelBridge.isCommApp(name)) {
                                        name += " (Comm)"
                                    }
                                    return name
                                }
                            }

                            Slider {
                                id: volumeSlider
                                from: 0
                                to: 100
                                enabled: !UserSettings.chatMixEnabled && !muteRoundButton.highlighted
                                opacity: enabled ? 1 : 0.5
                                Layout.fillWidth: true

                                // Bind directly to chatmix value and calculate the display volume
                                value: {
                                    if (!UserSettings.chatMixEnabled) {
                                        return applicationUnitLayout.model.volume
                                    }

                                    // Check if this app is a comm app
                                    let appName = applicationUnitLayout.model.name
                                    let isCommApp = SoundPanelBridge.isCommApp(appName)
                                    let chatMixVal = UserSettings.chatMixValue

                                    // Calculate the volume based on chatmix position
                                    if (chatMixVal <= 50) {
                                        // Left side: comm apps at 100%, other apps fade in
                                        return isCommApp ? 100 : (chatMixVal * 2)
                                    } else {
                                        // Right side: other apps at 100%, comm apps fade out
                                        return isCommApp ? (100 - (chatMixVal - 50) * 2) : 100
                                    }
                                }

                                ToolTip {
                                    parent: volumeSlider.handle
                                    visible: volumeSlider.pressed && (UserSettings.volumeValueMode === 0)
                                    text: Math.round(volumeSlider.value).toString()
                                }

                                onValueChanged: {
                                    if (!UserSettings.chatMixEnabled && pressed) {
                                        // Update the model using the correct index
                                        appModel.setProperty(applicationUnitLayout.index, "volume", value)

                                        // Then send to audio manager
                                        SoundPanelBridge.onApplicationVolumeSliderValueChanged(applicationUnitLayout.model.appID, value)
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
                Layout.preferredHeight: 1
                Layout.fillWidth: true
                color: Constants.separatorColor
                opacity: 0.15
                Layout.rightMargin: -14
                Layout.leftMargin: -14
            }

            ColumnLayout {
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

                        Component.onCompleted: {
                            palette.accent = palette.button
                        }

                        onClicked: {
                            if (!checked) {
                                UserSettings.chatMixEnabled = !checked
                                SoundPanelBridge.saveOriginalVolumesAfterRefresh()
                            } else {
                                UserSettings.chatMixEnabled = !checked
                                SoundPanelBridge.restoreOriginalVolumes()
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

                            ToolTip {
                                parent: chatMixSlider.handle
                                visible: chatMixSlider.pressed
                                text: {
                                    if (chatMixSlider.value === 0) return qsTr("Comm Apps Only")
                                    if (chatMixSlider.value === 100) return qsTr("Other Apps Only")
                                    return qsTr("Balanced: %1").arg(Math.round(chatMixSlider.value))
                                }
                            }

                            onValueChanged: {
                                UserSettings.chatMixValue = value
                            }

                            onPressedChanged: {
                                if (pressed) return

                                SoundPanelBridge.chatMixValue = UserSettings.chatMixValue
                                if (UserSettings.chatMixEnabled) {
                                    SoundPanelBridge.applyChatMixToApplications()
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

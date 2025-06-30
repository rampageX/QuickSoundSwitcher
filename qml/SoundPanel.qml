pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.FluentWinUI3
import QtQuick.Window
import Odizinne.QuickSoundSwitcher

ApplicationWindow {
    id: panel
    width: 360
    height: 200  // Initial height
    visible: false
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    color: "transparent"
    property bool isAnimatingIn: false
    property bool isAnimatingOut: false
    property int margin: UserSettings.panelMargin
    property int taskbarHeight: UserSettings.taskbarOffset
    property bool dataLoaded: false
    property bool darkMode: SoundPanelBridge.getDarkMode()
    property string taskbarPos: SoundPanelBridge.taskbarPosition

    signal hideAnimationFinished()
    signal showAnimationFinished()
    signal hideAnimationStarted()

    onShowAnimationFinished: {
        SoundPanelBridge.startMediaMonitoring()
    }

    onHideAnimationStarted: {
        SoundPanelBridge.stopMediaMonitoring()
    }

    PropertyAnimation {
        id: showAnimation
        target: panel
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
        duration: 210
        easing.type: Easing.InCubic
        onFinished: {
            panel.visible = false
            panel.isAnimatingOut = false
            panel.dataLoaded = false  // Reset for next time
            panel.hideAnimationFinished()
        }
    }

    function showPanel() {
        if (isAnimatingIn || isAnimatingOut) {
            return
        }

        isAnimatingIn = true
        panel.darkMode = SoundPanelBridge.getDarkMode()
        panel.taskbarPos = SoundPanelBridge.taskbarPosition
        panel.visible = true

        // Position the panel off-screen based on taskbar position
        positionPanelOffScreen()
    }

    function positionPanelOffScreen() {
        const screenWidth = Qt.application.screens[0].width
        const screenHeight = Qt.application.screens[0].height
        let startX, startY

        switch (panel.taskbarPos) {
        case "top":
            startX = screenWidth - width - margin
            startY = -height
            break
        case "bottom":
            startX = screenWidth - width - margin
            startY = screenHeight
            break
        case "left":
            startX = -width
            startY = screenHeight - height - margin
            break
        case "right":
            startX = screenWidth
            startY = screenHeight - height - margin
            break
        default:
            startX = screenWidth - width - margin
            startY = screenHeight
            break
        }

        panel.x = startX
        panel.y = startY
    }

    function startAnimation() {
        if (!isAnimatingIn) return

        const screenWidth = Qt.application.screens[0].width
        const screenHeight = Qt.application.screens[0].height
        let targetX, targetY

        switch (panel.taskbarPos) {
        case "top":
            targetX = screenWidth - width - margin
            targetY = margin + taskbarHeight
            showAnimation.property = "y"
            showAnimation.from = panel.y
            showAnimation.to = targetY
            break
        case "bottom":
            targetX = screenWidth - width - margin
            targetY = screenHeight - height - margin - taskbarHeight
            showAnimation.property = "y"
            showAnimation.from = panel.y
            showAnimation.to = targetY
            break
        case "left":
            targetX = taskbarHeight
            targetY = screenHeight - height - margin
            showAnimation.property = "x"
            showAnimation.from = panel.x
            showAnimation.to = targetX
            break
        case "right":
            targetX = screenWidth - width - taskbarHeight
            targetY = screenHeight - height - margin
            showAnimation.property = "x"
            showAnimation.from = panel.x
            showAnimation.to = targetX
            break
        default:
            targetX = screenWidth - width - margin
            targetY = screenHeight - height - margin - taskbarHeight
            showAnimation.property = "y"
            showAnimation.from = panel.y
            showAnimation.to = targetY
            break
        }

        // For left/right positioning, also set the Y coordinate immediately
        if (panel.taskbarPos === "left" || panel.taskbarPos === "right") {
            panel.y = targetY
        }
        // For top/bottom positioning, also set the X coordinate immediately
        if (panel.taskbarPos === "top" || panel.taskbarPos === "bottom") {
            panel.x = targetX
        }

        showAnimation.start()
    }

    function hidePanel() {
        if (isAnimatingIn || isAnimatingOut) {
            return
        }

        isAnimatingOut = true
        panel.hideAnimationStarted()

        let targetPos

        switch (panel.taskbarPos) {
        case "top":
            targetPos = -height
            hideAnimation.property = "y"
            hideAnimation.from = panel.y
            hideAnimation.to = targetPos
            break
        case "bottom":
            targetPos = Qt.application.screens[0].height
            hideAnimation.property = "y"
            hideAnimation.from = panel.y
            hideAnimation.to = targetPos
            break
        case "left":
            targetPos = -width
            hideAnimation.property = "x"
            hideAnimation.from = panel.x
            hideAnimation.to = targetPos
            break
        case "right":
            targetPos = Qt.application.screens[0].width
            hideAnimation.property = "x"
            hideAnimation.from = panel.x
            hideAnimation.to = targetPos
            break
        default:
            targetPos = Qt.application.screens[0].height
            hideAnimation.property = "y"
            hideAnimation.from = panel.y
            hideAnimation.to = targetPos
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

            panel.updatePanelHeight()
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
            // All data loaded - now start the animation
            panel.dataLoaded = true
            Qt.callLater(panel.startAnimation)
        }
    }

    function updatePanelHeight() {
        Qt.callLater(function() {
            panel.height = mediaLayout.implicitHeight + spacer.height + mainLayout.implicitHeight + 30 + 15
        })
    }

    KeepAlive {}

    SettingsWindow {
        id: settingsWindow
    }

    Rectangle {
        anchors.fill: mainLayout
        anchors.margins: -15
        color: panel.darkMode ? "#242424" : "#f2f2f2"
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
        color: panel.darkMode ? "#242424" : "#f2f2f2"
        visible: UserSettings.mediaMode === 0
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
        id: mediaLayout
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 15
        spacing: 10
        visible: UserSettings.mediaMode === 0

        ColumnLayout {
            RowLayout {
                id: infosLyt
                ColumnLayout {
                    Label {
                        text: SoundPanelBridge.mediaTitle || ""
                        font.pixelSize: 14
                        font.bold: true
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Label {
                        text: SoundPanelBridge.mediaArtist || ""
                        font.pixelSize: 12
                        opacity: 0.7
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }
                Image {
                    Layout.preferredWidth: 48
                    Layout.preferredHeight: 48
                    Layout.alignment: Qt.AlignVCenter
                    source: SoundPanelBridge.mediaArt || ""
                    fillMode: Image.PreserveAspectCrop
                    visible: SoundPanelBridge.mediaArt !== ""
                }
            }

            RowLayout {
                Item {
                    Layout.fillWidth: true
                }

                ToolButton {
                    text: "b"
                    onClicked: SoundPanelBridge.previousTrack()
                    Layout.preferredWidth: 40
                    Layout.preferredHeight: 40
                }

                ToolButton {
                    text: "b"
                    onClicked: SoundPanelBridge.playPause()
                    Layout.preferredWidth: 40
                    Layout.preferredHeight: 40
                }

                ToolButton {
                    text: "b"
                    onClicked: SoundPanelBridge.nextTrack()
                    Layout.preferredWidth: 40
                    Layout.preferredHeight: 40
                }

                Item {
                    Layout.fillWidth: true
                }
            }
        }
    }

    Item {
        id: spacer
        anchors.top: mediaLayout.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 30
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

            ComboBox {
                id: outputDeviceComboBox
                Layout.preferredHeight: 40
                Layout.fillWidth: true
                flat: true
                font.pixelSize: 15
                model: playbackDeviceModel
                textRole: UserSettings.deviceShortName ? "shortName" : "name"
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
                    value: pressed ? value : SoundPanelBridge.playbackVolume
                    from: 0
                    to: 100
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40

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

                Label {
                    text: Math.round(outputSlider.value).toString()
                    Layout.rightMargin: 5
                    font.pixelSize: 14
                    visible: UserSettings.volumeValueMode === 1
                }
            }

            ComboBox {
                id: inputDeviceComboBox
                Layout.topMargin: -2
                Layout.preferredHeight: 40
                Layout.fillWidth: true
                flat: true
                font.pixelSize: 15
                model: recordingDeviceModel
                textRole: UserSettings.deviceShortName ? "shortName" : "name"
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
                    value: pressed ? value : SoundPanelBridge.recordingVolume
                    from: 0
                    to: 100
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40

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

                Label {
                    text: Math.round(inputSlider.value).toString()
                    Layout.rightMargin: 5
                    font.pixelSize: 14
                    visible: UserSettings.volumeValueMode === 1
                }
            }
        }

        Rectangle {
            Layout.preferredHeight: 1
            Layout.fillWidth: true
            color: panel.darkMode ? "#E3E3E3" : "#A0A0A0"
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
                        icon.color: applicationUnitLayout.model.name === "Windows system sounds" ? (panel.darkMode ? "white" : "black") : "transparent"
                        onClicked: {
                            applicationUnitLayout.model.isMuted = !applicationUnitLayout.model.isMuted
                            SoundPanelBridge.onApplicationMuteButtonClicked(applicationUnitLayout.model.appID, applicationUnitLayout.model.isMuted)
                        }

                        Component.onCompleted: {
                            palette.accent = palette.button
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

                        ToolTip {
                            parent: volumeSlider.handle
                            visible: volumeSlider.pressed && (UserSettings.volumeValueMode === 0)
                            text: Math.round(volumeSlider.value).toString()
                        }

                        onValueChanged: {
                            SoundPanelBridge.onApplicationVolumeSliderValueChanged(applicationUnitLayout.model.appID, value)
                        }
                    }

                    Label {
                        text: Math.round(volumeSlider.value).toString()
                        Layout.rightMargin: 5
                        font.pixelSize: 14
                        visible: UserSettings.volumeValueMode === 1
                    }
                }
            }
        }

        Rectangle {
            color: panel.darkMode ? "#1c1c1c" : "#eeeeee"
            Layout.fillWidth: true
            Layout.fillHeight: true
            bottomLeftRadius: 12
            bottomRightRadius: 12
            Layout.preferredHeight: 50
            Layout.leftMargin: -14
            Layout.rightMargin: -14
            Layout.bottomMargin: -14

            Rectangle {
                height: 1
                color: panel.darkMode ? "#0F0F0F" : "#A0A0A0"
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

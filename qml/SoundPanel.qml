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

    onVisibleChanged: {
        if (!visible) {
            outputDevicesList.expanded = false
            inputDevicesList.expanded = false
        }
    }

    // Simplified ListView animations
    ParallelAnimation {
        id: outputExpandAnimation
        property int targetHeight: 0

        PropertyAnimation {
            target: outputDevicesRect
            property: "Layout.preferredHeight"
            from: 0
            to: outputExpandAnimation.targetHeight + 20
            duration: 150
            easing.type: Easing.OutQuad
        }
        PropertyAnimation {
            target: panel
            property: "height"
            from: panel.height
            to: panel.height + outputExpandAnimation.targetHeight + 20
            duration: 150
            easing.type: Easing.OutQuad
        }
    }

    ParallelAnimation {
        id: outputCollapseAnimation
        property int previousHeight: 0

        PropertyAnimation {
            target: outputDevicesRect
            property: "Layout.preferredHeight"
            to: 0
            duration: 150
            easing.type: Easing.OutQuad
        }
        PropertyAnimation {
            target: panel
            property: "height"
            to: panel.height - outputCollapseAnimation.previousHeight
            duration: 150
            easing.type: Easing.OutQuad
        }
    }

    ParallelAnimation {
        id: inputExpandAnimation
        property int targetHeight: 0

        PropertyAnimation {
            target: inputDevicesRect
            property: "Layout.preferredHeight"
            from: 0
            to: inputExpandAnimation.targetHeight + 20
            duration: 150
            easing.type: Easing.OutQuad
        }
        PropertyAnimation {
            target: panel
            property: "height"
            from: panel.height
            to: panel.height + inputExpandAnimation.targetHeight + 20
            duration: 150
            easing.type: Easing.OutQuad
        }
    }

    ParallelAnimation {
        id: inputCollapseAnimation
        property int previousHeight: 0

        PropertyAnimation {
            target: inputDevicesRect
            property: "Layout.preferredHeight"
            to: 0
            duration: 150
            easing.type: Easing.OutQuad
        }
        PropertyAnimation {
            target: panel
            property: "height"
            to: panel.height - inputCollapseAnimation.previousHeight
            duration: 150
            easing.type: Easing.OutQuad
        }
    }

    onHeightChanged: {
        // Reposition if we're in the middle of showing
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

    // Transform-based animations
    PropertyAnimation {
        id: showAnimation
        target: contentTransform
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
        target: contentTransform
        duration: 210
        easing.type: Easing.InCubic
        onFinished: {
            panel.visible = false
            panel.isAnimatingOut = false
            panel.dataLoaded = false
            panel.hideAnimationFinished()
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
        panel.darkMode = SoundPanelBridge.getDarkMode()
        panel.taskbarPos = SoundPanelBridge.taskbarPosition
        panel.visible = true

        // Position window at final target location
        positionPanelAtTarget()

        // Set initial transform (content off-screen)
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

        // Animate transform back to 0,0 (content slides into view)
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

        // Animate content off-screen
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
            // All data loaded - now calculate height and start animation
            panel.dataLoaded = true

            // Ensure panel height is calculated before animation
            Qt.callLater(function() {
                Qt.callLater(function() {
                    // Force height calculation to complete first
                    const newHeight = mediaLayout.implicitHeight + spacer.height + mainLayout.implicitHeight + 30 + 15
                    panel.height = newHeight

                    // Now start the animation with correct dimensions
                    Qt.callLater(panel.startAnimation)
                })
            })
        }
    }

    function updatePanelHeight() {
        Qt.callLater(function() {
            Qt.callLater(function() {
                const oldHeight = panel.height
                const newHeight = mediaLayout.implicitHeight + spacer.height + mainLayout.implicitHeight + 30 + 15

                // Only update height and reposition if needed
                if (panel.visible && !panel.isAnimatingIn && !panel.isAnimatingOut) {
                    panel.height = newHeight
                    positionPanelAtTarget() // Reposition to account for new height
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

    // Main content with transform applied
    Item {
        anchors.fill: parent
        transform: Translate {
            x: contentTransform.x
            y: contentTransform.y
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

        ColumnLayout {
            id: mediaLayout
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 15
            spacing: 10
            visible: UserSettings.mediaMode === 0 && (SoundPanelBridge.mediaTitle !== "")

            onImplicitHeightChanged: {
                if (panel.visible) {
                    panel.updatePanelHeight()
                }
            }
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
                        Layout.preferredWidth: 64
                        Layout.preferredHeight: 64
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
                        icon.source: "qrc:/icons/prev.png"
                        onClicked: SoundPanelBridge.previousTrack()
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                    }

                    ToolButton {
                        icon.source: SoundPanelBridge.isMediaPlaying ? "qrc:/icons/pause.png" : "qrc:/icons/play.png"
                        onClicked: SoundPanelBridge.playPause()
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                    }

                    ToolButton {
                        icon.source: "qrc:/icons/next.png"
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
                        text: outputDevicesList.expanded ? "−" : "+"
                        font.pixelSize: 16
                        font.bold: true
                        visible: playbackDeviceModel.count > 1
                        onClicked: {
                            outputDevicesList.expanded = !outputDevicesList.expanded
                        }
                    }
                }

                Rectangle {
                    id: outputDevicesRect
                    Layout.fillWidth: true
                    Layout.preferredHeight: 0
                    color: panel.darkMode ? "#1c1c1c" : "#eeeeee"
                    radius: 8
                    Rectangle {
                        anchors.fill: parent
                        color: "transparent"
                        radius: 12
                        border.width: 1
                        border.color: "#E3E3E3"
                        opacity: 0.1
                    }

                    ListView {
                        id: outputDevicesList
                        anchors.fill: parent
                        anchors.margins: 10
                        clip: true
                        interactive: false
                        property bool expanded: false
                        model: playbackDeviceModel

                        onExpandedChanged: {
                            if (expanded) {
                                outputExpandAnimation.targetHeight = contentHeight
                                outputExpandAnimation.start()
                            } else {
                                outputCollapseAnimation.previousHeight = parent.Layout.preferredHeight
                                outputCollapseAnimation.start()
                            }
                        }

                        delegate: ItemDelegate {
                            width: outputDevicesList.width
                            height: 40
                            required property var model
                            required property string name
                            required property string shortName
                            required property bool isDefault
                            required property string id
                            required property int index
                            highlighted: model.isDefault
                            text: UserSettings.deviceShortName ? model.shortName : model.name

                            onClicked: {
                                // Update output model to reflect new default device
                                for (let i = 0; i < playbackDeviceModel.count; i++) {
                                    playbackDeviceModel.setProperty(i, "isDefault", i === index)
                                }
                                SoundPanelBridge.onPlaybackDeviceChanged(name)
                                if (UserSettings.linkIO) {
                                    // Use the same name format that's being displayed
                                    const selectedDeviceName = UserSettings.deviceShortName ? shortName : name
                                    // Find matching input device by name
                                    let linkedInputIndex = -1
                                    for (let i = 0; i < recordingDeviceModel.count; i++) {
                                        const inputName = UserSettings.deviceShortName ?
                                            recordingDeviceModel.get(i).shortName :
                                            recordingDeviceModel.get(i).name
                                        if (inputName === selectedDeviceName) {
                                            linkedInputIndex = i
                                            break
                                        }
                                    }
                                    // If matching input device found, update input model
                                    if (linkedInputIndex !== -1) {
                                        for (let i = 0; i < recordingDeviceModel.count; i++) {
                                            recordingDeviceModel.setProperty(i, "isDefault", i === linkedInputIndex)
                                        }
                                    }
                                    SoundPanelBridge.onRecordingDeviceChanged(name)

                                    if (UserSettings.closeDeviceListOnClick) {
                                        outputDevicesList.expanded = false
                                    }
                                }
                            }
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
                        text: inputDevicesList.expanded ? "−" : "+"
                        font.pixelSize: 16
                        font.bold: true
                        visible: recordingDeviceModel.count > 1
                        onClicked: {
                            inputDevicesList.expanded = !inputDevicesList.expanded
                        }
                    }
                }

                Rectangle {
                    id: inputDevicesRect
                    Layout.fillWidth: true
                    Layout.preferredHeight: 0
                    color: panel.darkMode ? "#1c1c1c" : "#eeeeee"
                    radius: 12
                    Rectangle {
                        anchors.fill: parent
                        color: "transparent"
                        radius: 12
                        border.width: 1
                        border.color: "#E3E3E3"
                        opacity: 0.1
                    }

                    ListView {
                        id: inputDevicesList
                        anchors.fill: parent
                        anchors.margins: 10
                        clip: true
                        interactive: false
                        property bool expanded: false
                        model: recordingDeviceModel

                        onExpandedChanged: {
                            if (expanded) {
                                inputExpandAnimation.targetHeight = contentHeight
                                inputExpandAnimation.start()
                            } else {
                                inputCollapseAnimation.previousHeight = parent.Layout.preferredHeight
                                inputCollapseAnimation.start()
                            }
                        }

                        delegate: ItemDelegate {
                            width: inputDevicesList.width
                            height: 40

                            required property string name
                            required property string shortName
                            required property bool isDefault
                            required property string id
                            required property int index
                            required property var model

                            highlighted: model.isDefault
                            text: UserSettings.deviceShortName ? model.shortName : model.name
                            onClicked: {
                                // Update input model to reflect new default device
                                for (let i = 0; i < recordingDeviceModel.count; i++) {
                                    recordingDeviceModel.setProperty(i, "isDefault", i === index)
                                }

                                SoundPanelBridge.onRecordingDeviceChanged(name)

                                if (UserSettings.linkIO) {
                                    // Use the same name format that's being displayed
                                    const selectedDeviceName = UserSettings.deviceShortName ? shortName : name

                                    // Find matching output device by name
                                    let linkedOutputIndex = -1
                                    for (let i = 0; i < playbackDeviceModel.count; i++) {
                                        const outputName = UserSettings.deviceShortName ?
                                            playbackDeviceModel.get(i).shortName :
                                            playbackDeviceModel.get(i).name

                                        if (outputName === selectedDeviceName) {
                                            linkedOutputIndex = i
                                            break
                                        }
                                    }

                                    // If matching output device found, update output model
                                    if (linkedOutputIndex !== -1) {
                                        for (let i = 0; i < playbackDeviceModel.count; i++) {
                                            playbackDeviceModel.setProperty(i, "isDefault", i === linkedOutputIndex)
                                        }
                                    }

                                    SoundPanelBridge.onPlaybackDeviceChanged(name)

                                    if (UserSettings.closeDeviceListOnClick) {
                                        inputDevicesList.expanded = false
                                    }
                                }
                            }
                        }
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

                        ColumnLayout {
                            spacing: -4
                            Label {
                                visible: UserSettings.displayDevAppLabel
                                opacity: 0.5
                                elide: Text.ElideRight
                                Layout.preferredWidth: 200
                                Layout.leftMargin: 18
                                Layout.rightMargin: 25
                                text: applicationUnitLayout.model.name
                            }

                            Slider {
                                id: volumeSlider
                                from: 0
                                to: 100
                                enabled: !muteRoundButton.highlighted
                                value: applicationUnitLayout.model.volume
                                Layout.fillWidth: true

                                ToolTip {
                                    parent: volumeSlider.handle
                                    visible: volumeSlider.pressed && (UserSettings.volumeValueMode === 0)
                                    text: Math.round(volumeSlider.value).toString()
                                }

                                onValueChanged: {
                                    SoundPanelBridge.onApplicationVolumeSliderValueChanged(applicationUnitLayout.model.appID, value)
                                }
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
                Layout.fillHeight: false
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
}

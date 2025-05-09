import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

ApplicationWindow {
    id: window
    width: 400
    height: mainLayout.implicitHeight
    visible: false
    flags: Qt.FramelessWindowHint
    color: "transparent"
    Material.theme: Material.System
    property bool blockOutputSignal: false
    property bool blockInputSignal: false

    ListModel {
        id: appModel
    }

    function clearApplicationModel() {
        appModel.clear();
    }

    function addApplication(appID, name, isMuted, volume, icon) {
        appModel.append({
                            appID: appID,
                            name: name,
                            isMuted: isMuted,
                            volume: volume,
                            icon: "data:image/png;base64," + icon
                        });
    }

    function setOutputImageSource(source) {
        outputImage.source = source;
    }

    function setInputImageSource(source) {
        inputImage.source = source;
    }

    function clearPlaybackDevices() {
        outputDeviceComboBox.model.clear();
    }

    function addPlaybackDevice(deviceName) {
        outputDeviceComboBox.model.append({name: deviceName});
    }

    function clearRecordingDevices() {
        inputDeviceComboBox.model.clear();
    }

    function addRecordingDevice(deviceName) {
        inputDeviceComboBox.model.append({name: deviceName});
    }

    function setPlaybackDeviceCurrentIndex(index) {
        blockOutputSignal = true;
        outputDeviceComboBox.currentIndex = index;
        blockOutputSignal = false;
    }

    function setRecordingDeviceCurrentIndex(index) {
        blockInputSignal = true;
        inputDeviceComboBox.currentIndex = index;
        blockInputSignal = false;
    }

    Rectangle {
        anchors.fill: parent
        color: Material.theme === Material.Dark ? "#1C1C1C" : "#E3E3E3"
        radius: Material.MediumScale
        Rectangle {
            anchors.fill: parent
            color: "transparent"
            radius: Material.MediumScale
            border.width: 1
            border.color: Material.accent
            opacity: 0.5
        }
    }

    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        anchors.margins: 15
        spacing: 15

        Label {
            text: qsTr("Audio devices")
            Layout.bottomMargin: -10
            Layout.leftMargin: 10
            color: Material.accent
        }

        Pane {
            Layout.fillWidth: true
            Material.background: Material.theme === Material.Dark ? "#2B2B2B" : "#FFFFFF"
            Material.elevation: 6
            Material.roundedScale: Material.MediumScale
            ColumnLayout {
                id: deviceLayout
                objectName: "gridLayout"
                anchors.fill: parent
                spacing: 5

                ComboBox {
                    id: outputDeviceComboBox
                    visible: !mixerOnly
                    Layout.preferredHeight: 40
                    Layout.columnSpan: 3
                    Layout.fillWidth: true
                    flat: true
                    font.pixelSize: 15
                    model: ListModel {}
                    contentItem: Text {
                        text: outputDeviceComboBox.currentText
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignLeft
                        font.pixelSize: 15
                        leftPadding: 10
                        width: parent.width
                        color: textColor
                    }
                    onCurrentTextChanged: {
                        if (!window.blockOutputSignal) {
                            soundPanel.onPlaybackDeviceChanged(outputDeviceComboBox.currentText);
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    RoundButton {
                        id: outputeMuteRoundButton
                        visible: !mixerOnly
                        Layout.preferredHeight: 40
                        Layout.preferredWidth: 40
                        flat: true
                        icon.source: outputIcon
                        onClicked: {
                            soundPanel.onOutputMuteRoundButtonClicked()
                        }
                    }

                    Slider {
                        id: outputSlider
                        visible: !mixerOnly
                        value: soundPanel.playbackVolume
                        from: 0
                        to: 100
                        stepSize: 1
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        onValueChanged: {
                            if (pressed) {
                                soundPanel.onPlaybackVolumeChanged(value)
                            }
                        }

                        onPressedChanged: {
                            if (!pressed) {
                                soundPanel.onOutputSliderReleased()
                            }
                        }
                    }
                }

                ComboBox {
                    id: inputDeviceComboBox
                    visible: !mixerOnly
                    Layout.topMargin: -2
                    Layout.preferredHeight: 40
                    Layout.fillWidth: true
                    Layout.columnSpan: 3
                    flat: true
                    font.pixelSize: 15
                    model: ListModel {}
                    contentItem: Text {
                        text: inputDeviceComboBox.currentText
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignLeft
                        font.pixelSize: 15
                        width: parent.width
                        color: textColor
                        leftPadding: 10
                    }
                    onCurrentTextChanged: {
                        if (!window.blockInputSignal) {
                            soundPanel.onRecordingDeviceChanged(inputDeviceComboBox.currentText);
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    RoundButton {
                        id: inputMuteRoundButton
                        visible: !mixerOnly
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        flat: true
                        icon.source: inputIcon
                        onClicked: {
                            soundPanel.onInputMuteRoundButtonClicked()
                        }
                    }

                    Slider {
                        id: inputSlider
                        visible: !mixerOnly
                        value: soundPanel.recordingVolume
                        from: 0
                        to: 100
                        stepSize: 1
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        onValueChanged: {
                            soundPanel.onRecordingVolumeChanged(value)
                        }
                    }
                }
            }
        }

        Label {
            text: qsTr("Applications")
            Layout.bottomMargin: -10
            Layout.leftMargin: 10
            color: Material.accent
        }

        Pane {
            Layout.fillWidth: true
            Material.background: Material.theme === Material.Dark ? "#2B2B2B" : "#FFFFFF"
            Material.elevation: 6
            Material.roundedScale: Material.MediumScale
            ColumnLayout {
                id: appLayout
                objectName: "gridLayout"
                anchors.fill: parent
                spacing: 5

                Repeater {
                    id: appRepeater
                    model: appModel
                    onCountChanged: {
                        window.height = mainLayout.implicitHeight + ((45 * appRepeater.count) - 5) + 30
                    }
                    delegate: RowLayout {
                        id: applicationUnitLayout
                        Layout.preferredHeight: 40
                        Layout.fillWidth: true

                        RoundButton {
                            id: muteRoundButton
                            Layout.preferredWidth: 40
                            Layout.preferredHeight: 40
                            flat: true
                            checkable: true
                            checked: model.isMuted
                            ToolTip.text: model.name
                            ToolTip.visible: hovered
                            ToolTip.delay: 1000
                            icon.source: model.name === "Windows system sounds" ? systemSoundsIcon : model.icon
                            icon.color: model.name === "Windows system sounds" ? undefined : "transparent"
                            onClicked: {
                                model.isMuted = !model.isMuted;
                                soundPanel.onApplicationMuteRoundButtonClicked(model.appID, model.isMuted);
                            }
                        }

                        Slider {
                            id: volumeSlider
                            from: 0
                            to: 100
                            stepSize: 1
                            value: model.volume
                            Layout.fillWidth: true
                            Layout.preferredHeight: 40
                            onValueChanged: {
                                soundPanel.onApplicationVolumeSliderValueChanged(model.appID, value);
                            }
                        }
                    }
                }
            }
        }
    }
}

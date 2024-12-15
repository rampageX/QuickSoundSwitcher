import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls.FluentWinUI3 2.15

ApplicationWindow {
    id: window
    width: 330
    height: gridLayout.implicitHeight
    visible: false
    flags: Qt.FramelessWindowHint
    color: "transparent"

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
        id: ioRectangle
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.left: parent.left
        height: 182
        color: nativeWindowColor
        border.color: Qt.rgba(255, 255, 255, 0.15)
        border.width: 1
        radius: 12
    }

    Rectangle {
        id: appRectangle
        anchors.fill: parent
        anchors.topMargin: 193
        color: singleApp ? contrastedColor : nativeWindowColor
        border.color: Qt.rgba(255, 255, 255, 0.15)
        border.width: 1
        radius: 12


        Rectangle {
            id: bottomContrastRectangle
            anchors.top: appRepeater.bottom
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 1
            anchors.leftMargin: 1
            anchors.rightMargin: 1
            anchors.bottomMargin: 1
            height: 51
            visible: !singleApp
            //border.color: Qt.rgba(255, 255, 255, 0.15)
            //border.width: 1
            radius: 12
            color: contrastedColor

            Rectangle {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 1
                anchors.rightMargin: 1
                height: 12
                color: contrastedColor

                Rectangle {
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: 1
                    anchors.rightMargin: 1
                    height: 1
                    color: contrastedBorderColor
                }
            }
        }
    }

    GridLayout {
        id: gridLayout
        objectName: "gridLayout"
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        anchors.topMargin: 10
        anchors.bottomMargin: 10
        columns: 2
        rowSpacing: 5

        ComboBox {
            id: outputDeviceComboBox
            Layout.preferredHeight: 35
            Layout.columnSpan: 2
            Layout.fillWidth: true
            font.pixelSize: 14
            model: ListModel {}
            onCurrentTextChanged: {
                if (!window.blockOutputSignal) {
                    soundPanel.onPlaybackDeviceChanged(outputDeviceComboBox.currentText);
                }
            }
        }

        Button {
            id: outputeMuteButton
            Layout.preferredHeight: 40
            Layout.preferredWidth: 40
            flat: true
            onClicked: {
                soundPanel.onOutputMuteButtonClicked()
            }

            Image {
                id: outputImage
                width: 16
                height: 16
                anchors.margins: 0
                anchors.centerIn: parent
                source: outputIcon
                fillMode: Image.PreserveAspectFit
            }
        }

        Slider {
            id: outputSlider
            value: soundPanel.playbackVolume
            from: 0
            to: 100
            Layout.leftMargin: -15
            Layout.rightMargin: -5
            Layout.fillWidth: true
            Layout.preferredHeight: -1
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

        ComboBox {
            id: inputDeviceComboBox
            Layout.preferredHeight: 35
            Layout.fillWidth: true
            Layout.columnSpan: 2
            font.pixelSize: 14
            model: ListModel {}
            onCurrentTextChanged: {
                if (!window.blockInputSignal) {
                    soundPanel.onRecordingDeviceChanged(inputDeviceComboBox.currentText);
                }
            }
        }

        Button {
            id: inputMuteButton
            Layout.preferredWidth: 40
            Layout.preferredHeight: 40
            flat: true
            onClicked: {
                soundPanel.onInputMuteButtonClicked()
            }
            Image {
                id: inputImage
                width: 16
                height: 16
                anchors.margins: 0
                anchors.centerIn: parent
                source: inputIcon
                fillMode: Image.PreserveAspectFit
            }
        }

        Slider {
            id: inputSlider
            value: soundPanel.recordingVolume
            from: 0
            to: 100
            Layout.leftMargin: -15
            Layout.rightMargin: -5
            Layout.fillWidth: true
            Layout.preferredHeight: -1
            onValueChanged: {
                soundPanel.onRecordingVolumeChanged(value)
            }
        }

        Item {
            Layout.preferredHeight: 18
            Layout.columnSpan: 2
        }

        Repeater {
            id: appRepeater
            model: appModel
            onCountChanged: {
                window.height = gridLayout.implicitHeight + (45 * appRepeater.count)
            }
            delegate: RowLayout {
                Layout.fillWidth: true
                Layout.columnSpan: 2

                Button {
                    id: muteButton
                    Layout.preferredWidth: 40
                    Layout.preferredHeight: 40
                    flat: true
                    checkable: true
                    checked: model.isMuted
                    onClicked: {
                        let appIDs = model.appID.split(";");
                        appIDs.forEach(function(appID) {
                            soundPanel.onApplicationMuteButtonClicked(appID, muteButton.checked);
                        });
                    }

                    Image {
                        source: model.name === "Windows system sounds" ? systemSoundsIcon : model.icon
                        anchors.centerIn: parent
                        width: 16
                        height: 16
                    }
                }

                Slider {
                    id: volumeSlider
                    from: 0
                    to: 100
                    Layout.leftMargin: -15
                    Layout.rightMargin: -5
                    Layout.fillWidth: true
                    value: model.volume
                    onValueChanged: {
                        let appIDs = model.appID.split(";");
                        appIDs.forEach(function(appID) {
                            soundPanel.onApplicationVolumeSliderValueChanged(appID, volumeSlider.value);
                        });
                    }
                }
            }
        }

        Item {
            Layout.preferredHeight: 15
        }
    }
}

import QtQuick.Controls.FluentWinUI3
import QtQuick.Layouts
import QtQuick
import Odizinne.QuickSoundSwitcher

ColumnLayout {
    spacing: 3

    Label {
        text: qsTr("Appearance & Position")
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
            Card {
                Layout.fillWidth: true
                title: qsTr("Panel position")
                description: ""

                additionalControl: ComboBox {
                    Layout.preferredHeight: 35
                    Layout.preferredWidth: 160
                    model: [qsTr("Top"), qsTr("Bottom"), qsTr("Left"), qsTr("Right")]
                    currentIndex: UserSettings.panelPosition
                    onActivated: UserSettings.panelPosition = currentIndex
                }
            }

            Card {
                Layout.fillWidth: true
                title: qsTr("Taskbar offset")
                description: ""

                additionalControl: SpinBox {
                    Layout.preferredHeight: 35
                    Layout.preferredWidth: 160
                    from: 0
                    to: 200
                    editable: true
                    value: UserSettings.taskbarOffset
                    onValueModified: UserSettings.taskbarOffset = value
                }
            }

            Card {
                Layout.fillWidth: true
                title: qsTr("Panel margin")
                description: qsTr("How far the panel should be pushed from screen edge")

                additionalControl: SpinBox {
                    Layout.preferredHeight: 35
                    Layout.preferredWidth: 160
                    from: 0
                    to: 200
                    editable: true
                    value: UserSettings.panelMargin
                    onValueModified: UserSettings.panelMargin = value
                }
            }

            Card {
                Layout.fillWidth: true
                title: qsTr("Use short device names")
                description: qsTr("Shorten device names by shrinking description")

                additionalControl: Switch {
                    checked: UserSettings.deviceShortName
                    onClicked: UserSettings.deviceShortName = checked
                }
            }

            Card {
                Layout.fillWidth: true
                title: qsTr("Volume value display mode")
                description: qsTr("Control how you want sound value to be displayed")

                additionalControl: ComboBox {
                    Layout.preferredHeight: 35
                    Layout.preferredWidth: 160
                    model: [qsTr("Slider tooltip"), qsTr("Label"), qsTr("Hidden")]
                    currentIndex: UserSettings.volumeValueMode
                    onActivated: UserSettings.volumeValueMode = currentIndex
                }
            }

            Card {
                Layout.fillWidth: true
                title: qsTr("Media info display")
                description: qsTr("Display currently playing media from Windows known sources")

                additionalControl: ComboBox {
                    Layout.preferredHeight: 35
                    Layout.preferredWidth: 160
                    model: [qsTr("Flyout (interactive)"), qsTr("Panel (informative)"), qsTr("Hidden")]
                    currentIndex: UserSettings.mediaMode
                    onActivated: UserSettings.mediaMode = currentIndex
                }
            }

            Card {
                Layout.fillWidth: true
                title: qsTr("Applications and devices label")
                description: ""

                additionalControl: Switch {
                    checked: UserSettings.displayDevAppLabel
                    onClicked: UserSettings.displayDevAppLabel = checked
                }
            }

            Card {
                Layout.fillWidth: true
                title: qsTr("Show audio level")
                description: "Display audio level value in slider"

                additionalControl: Switch {
                    checked: UserSettings.showAudioLevel
                    onClicked: UserSettings.showAudioLevel = checked
                }
            }
        }
    }
}

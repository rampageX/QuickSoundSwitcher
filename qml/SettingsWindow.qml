pragma ComponentBehavior: Bound

import QtQuick.Controls.FluentWinUI3
import QtQuick.Layouts
import QtQuick
import Odizinne.QuickSoundSwitcher

ApplicationWindow {
    id: root
    height: 450
    minimumHeight: 450
    width: 900
    minimumWidth: 900
    visible: false
    transientParent: null
    title: qsTr("QuickSoundSwitcher - Settings")

    property int rowHeight: 35

    Popup {
        anchors.centerIn: parent
        width: mainPopupLyt.implicitWidth + 50
        height: implicitHeight + 20
        id: easterEggDialog
        modal: true
        ColumnLayout {
            id: mainPopupLyt
            spacing: 15
            anchors.fill: parent

            Label {
                text: "You just lost the game"
                font.bold: true
                font.pixelSize: 18
                Layout.alignment: Qt.AlignCenter
            }

            Button {
                text: "Too bad"
                onClicked: easterEggDialog.close()
                Layout.alignment: Qt.AlignCenter
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 15

        Item {
            Layout.preferredWidth: 200
            Layout.fillHeight: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 10

                ListView {
                    id: sidebarList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    interactive: false
                    model: [
                        {
                            text: qsTr("General"),
                            icon: "qrc:/icons/general.svg"
                        },
                        {
                            text: qsTr("Appearance"),
                            icon: "qrc:/icons/wand.svg"
                        },
                        {
                            text: qsTr("Language"),
                            icon: "qrc:/icons/language.svg"
                        },
                        {
                            text: qsTr("Debug"),
                            icon: "qrc:/icons/chip.svg"
                        }
                    ]
                    currentIndex: 0

                    delegate: ItemDelegate {
                        id: del
                        width: parent.width
                        height: 43
                        spacing: 10
                        required property var modelData
                        required property int index

                        property bool isCurrentItem: ListView.isCurrentItem

                        highlighted: ListView.isCurrentItem
                        icon.source: del.modelData.icon
                        text: del.modelData.text
                        icon.width: 18
                        icon.height: 18

                        onClicked: {
                            if (sidebarList.currentIndex !== index) {
                                sidebarList.currentIndex = index
                                switch(index) {
                                    case 0: stackView.push(generalPaneComponent); break
                                    case 1: stackView.push(appearancePaneComponent); break
                                    case 2: stackView.push(languagePaneComponent); break
                                    case 3: stackView.push(debugPaneComponent); break
                                }
                            }
                        }
                    }
                }
            }
        }

        StackView {
            id: stackView
            Layout.fillWidth: true
            Layout.fillHeight: true
            initialItem: generalPaneComponent

            popEnter: Transition {
                ParallelAnimation {
                    NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 150; easing.type: Easing.InQuint }
                    NumberAnimation { property: "y"; from: (stackView.mirrored ? -0.3 : 0.3) * -stackView.width; to: 0; duration: 300; easing.type: Easing.OutCubic }
                }
            }

            pushEnter: Transition {
                ParallelAnimation {
                    NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 150; easing.type: Easing.InQuint }
                    NumberAnimation { property: "y"; from: (stackView.mirrored ? -0.3 : 0.3) * stackView.width; to: 0; duration: 300; easing.type: Easing.OutCubic }
                }
            }

            popExit: Transition {
                NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 150; easing.type: Easing.OutQuint }
            }

            pushExit: Transition {
                NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 150; easing.type: Easing.OutQuint }
            }

            replaceEnter: Transition {
                ParallelAnimation {
                    NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 150; easing.type: Easing.InQuint }
                    NumberAnimation { property: "y"; from: (stackView.mirrored ? -0.3 : 0.3) * stackView.width; to: 0; duration: 300; easing.type: Easing.OutCubic }
                }
            }

            Component {
                id: generalPaneComponent

                ColumnLayout {
                    spacing: 3

                    Label {
                        text: qsTr("General Settings")
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
                                title: qsTr("Panel mode")
                                description: qsTr("Choose what should be displayed in the panel")

                                additionalControl: ComboBox {
                                    Layout.preferredHeight: 35
                                    Layout.preferredWidth: 160
                                    model: [qsTr("Devices + Mixer"), qsTr("Mixer only"), qsTr("Devices only")]
                                    currentIndex: UserSettings.panelMode
                                    onActivated: UserSettings.panelMode = currentIndex
                                }
                            }

                            Card {
                                Layout.fillWidth: true
                                title: qsTr("Link same input and output devices")
                                description: qsTr("Try to match input / output from the same device")

                                additionalControl: Switch {
                                    checked: UserSettings.linkIO
                                    onClicked: UserSettings.linkIO = checked
                                }
                            }

                            Card {
                                Layout.fillWidth: true
                                title: qsTr("Sound keepalive")
                                description: qsTr("Emit an inaudible sound to keep bluetooth devices awake")

                                additionalControl: Switch {
                                    checked: UserSettings.keepAlive
                                    onClicked: UserSettings.keepAlive = checked
                                }
                            }

                            Card {
                                Layout.fillWidth: true
                                title: qsTr("Run at system startup")
                                description: qsTr("QSS will boot up when your computer starts")

                                additionalControl: Switch {
                                    checked: SoundPanelBridge.getShortcutState()
                                    onClicked: SoundPanelBridge.setStartupShortcut(checked)
                                }
                            }

                            Card {
                                Layout.fillWidth: true
                                title: qsTr("Close device list automatically")
                                description: qsTr("Device list will automatically close after selecting a device")

                                additionalControl: Switch {
                                    checked: UserSettings.closeDeviceListOnClick
                                    onClicked: UserSettings.closeDeviceListOnClick = checked
                                }
                            }

                            Card {
                                Layout.fillWidth: true
                                title: qsTr("Group applications by executable")
                                description: qsTr("Control multiple stream comming from a single app with one slider")

                                additionalControl: Switch {
                                    checked: UserSettings.groupApplications
                                    onClicked: UserSettings.groupApplications = checked
                                }
                            }
                        }
                    }
                }
            }

            Component {
                id: appearancePaneComponent

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
                        }
                    }
                }
            }

            Component {
                id: languagePaneComponent

                ColumnLayout {
                    spacing: 3

                    Label {
                        text: qsTr("Debug and information")
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
                                title: qsTr("Application language")

                                additionalControl: ComboBox {
                                    Layout.preferredHeight: 35
                                    model: [qsTr("System"), "english", "français", "deutsch", "italiano", "한글", "简体中文"]
                                    currentIndex: UserSettings.languageIndex
                                    onActivated: {
                                        UserSettings.languageIndex = currentIndex
                                        SoundPanelBridge.changeApplicationLanguage(currentIndex)
                                        currentIndex = UserSettings.languageIndex
                                        Qt.callLater(function() {
                                            sidebarList.currentIndex = 2
                                        })
                                    }
                                }
                            }

                            Card {
                                id: trProgressCard
                                Layout.fillWidth: true
                                title: qsTr("Translation Progress")

                                additionalControl: ProgressBar {
                                    id: trProgressBar
                                    Layout.preferredWidth: 160
                                    Layout.preferredHeight: 6
                                    from: 0
                                    to: SoundPanelBridge.getTotalTranslatableStrings()
                                    value: SoundPanelBridge.getCurrentLanguageFinishedStrings(UserSettings.languageIndex)
                                }
                            }

                            Card {
                                Layout.fillWidth: true
                                title: qsTr("Translation author")
                                description: ""

                                additionalControl: Label {
                                    text: qsTr("Unknow author")
                                    opacity: 0.5
                                }
                            }

                            Card {
                                Layout.fillWidth: true
                                title: qsTr("Translation last updated")
                                description: ""

                                additionalControl: Label {
                                    text: qsTr("Unknow date")
                                    opacity: 0.5
                                }
                            }
                        }
                    }
                }
            }

            Component {
                id: debugPaneComponent

                ColumnLayout {
                    spacing: 3

                    Label {
                        text: qsTr("Debug and information")
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
                                title: qsTr("Application version")
                                description: ""

                                property int clickCount: 0

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        parent.clickCount++
                                        if (parent.clickCount >= 5) {
                                            easterEggDialog.open()
                                            parent.clickCount = 0
                                        }
                                    }
                                }
                                additionalControl: Label {
                                    text: SoundPanelBridge.getAppVersion()
                                    opacity: 0.5
                                }
                            }

                            Card {
                                Layout.fillWidth: true
                                title: qsTr("QT version")
                                description: ""

                                additionalControl: Label {
                                    text: SoundPanelBridge.getQtVersion()
                                    opacity: 0.5
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}


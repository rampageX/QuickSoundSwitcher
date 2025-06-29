pragma ComponentBehavior: Bound

import QtQuick.Controls.FluentWinUI3
import QtQuick.Controls.impl
import QtQuick.Layouts
import QtQuick
import Odizinne.QuickSoundSwitcher

ApplicationWindow {
    id: root
    height: 400
    minimumHeight: 400
    width: 700
    minimumWidth: 700
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
                                case 2: stackView.push(debugPaneComponent); break
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
                    NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 200; easing.type: Easing.InQuint }
                    NumberAnimation { property: "y"; from: (stackView.mirrored ? -0.3 : 0.3) * -stackView.width; to: 0; duration: 400; easing.type: Easing.OutCubic }
                }
            }

            pushEnter: Transition {
                ParallelAnimation {
                    NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 200; easing.type: Easing.InQuint }
                    NumberAnimation { property: "y"; from: (stackView.mirrored ? -0.3 : 0.3) * stackView.width; to: 0; duration: 400; easing.type: Easing.OutCubic }
                }
            }

            popExit: Transition {
                NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 200; easing.type: Easing.OutQuint }
            }

            pushExit: Transition {
                NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 200; easing.type: Easing.OutQuint }
            }

            replaceEnter: Transition {
                ParallelAnimation {
                    NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 200; easing.type: Easing.InQuint }
                    NumberAnimation { property: "y"; from: (stackView.mirrored ? -0.3 : 0.3) * stackView.width; to: 0; duration: 400; easing.type: Easing.OutCubic }
                }
            }

            Component {
                id: generalPaneComponent

                ColumnLayout {
                    spacing: 15

                    Label {
                        text: qsTr("General Settings")
                        font.pixelSize: 22
                        font.bold: true
                    }

                    RowLayout {
                        spacing: 15
                        Layout.preferredHeight: root.rowHeight
                        Label {
                            text: qsTr("Panel mode")
                            Layout.fillWidth: true
                        }

                        ComboBox {
                            Layout.preferredHeight: 35
                            Layout.preferredWidth: 200
                            model: [qsTr("Devices + Mixer"), qsTr("Mixer only"), qsTr("Devices only")]
                            currentIndex: UserSettings.panelMode
                            onActivated: UserSettings.panelMode = currentIndex
                        }
                    }

                    RowLayout {
                        spacing: 15
                        Layout.preferredHeight: root.rowHeight
                        Label {
                            text: qsTr("Link same input and output devices")
                            Layout.fillWidth: true
                        }

                        Switch {
                            checked: UserSettings.linkIO
                            onClicked: UserSettings.linkIO = checked
                        }
                    }

                    RowLayout {
                        spacing: 15
                        Layout.preferredHeight: root.rowHeight
                        Label {
                            text: qsTr("Sound keepalive")
                            Layout.fillWidth: true
                        }

                        Switch {
                            checked: UserSettings.keepAlive
                            onClicked: UserSettings.keepAlive = checked
                        }
                    }

                    RowLayout {
                        spacing: 15
                        Layout.preferredHeight: root.rowHeight
                        Label {
                            text: qsTr("Run at system startup")
                            Layout.fillWidth: true
                        }

                        Switch {
                            checked: SoundPanelBridge.getShortcutState()
                            onClicked: SoundPanelBridge.setStartupShortcut(checked)
                        }
                    }

                    Item {
                        Layout.fillHeight: true
                    }
                }
            }


            Component {
                id: appearancePaneComponent

                ColumnLayout {
                    spacing: 15

                    Label {
                        text: qsTr("Appearance & Position")
                        font.pixelSize: 22
                        font.bold: true
                    }

                    RowLayout {
                        spacing: 15
                        Layout.preferredHeight: root.rowHeight
                        Label {
                            text: qsTr("Panel position")
                            Layout.fillWidth: true
                        }
                        ComboBox {
                            Layout.preferredHeight: 35
                            Layout.preferredWidth: 200
                            model: [qsTr("Top"), qsTr("Bottom"), qsTr("Left"), qsTr("Right")]
                            currentIndex: UserSettings.panelPosition
                            onActivated: UserSettings.panelPosition = currentIndex
                        }
                    }

                    RowLayout {
                        spacing: 15
                        Layout.preferredHeight: root.rowHeight
                        Label {
                            text: qsTr("Taskbar offset")
                            Layout.fillWidth: true
                        }
                        SpinBox {
                            Layout.preferredHeight: 35
                            Layout.preferredWidth: 150
                            from: 0
                            to: 200
                            editable: true
                            value: UserSettings.taskbarOffset
                            onValueModified: UserSettings.taskbarOffset = value
                        }
                    }

                    RowLayout {
                        spacing: 15
                        Layout.preferredHeight: root.rowHeight
                        Label {
                            text: qsTr("Panel margin")
                            Layout.fillWidth: true
                        }
                        SpinBox {
                            Layout.preferredHeight: 35
                            Layout.preferredWidth: 150
                            from: 0
                            to: 200
                            editable: true
                            value: UserSettings.panelMargin
                            onValueModified: UserSettings.panelMargin = value
                        }
                    }

                    RowLayout {
                        spacing: 15
                        Layout.preferredHeight: root.rowHeight
                        Label {
                            text: qsTr("Use short device names")
                            Layout.fillWidth: true
                        }

                        Switch {
                            checked: UserSettings.deviceShortName
                            onClicked: UserSettings.deviceShortName = checked
                        }
                    }

                    RowLayout {
                        spacing: 15
                        Layout.preferredHeight: root.rowHeight
                        Label {
                            text: qsTr("Volume value display mode")
                            Layout.fillWidth: true
                        }
                        ComboBox {
                            Layout.preferredHeight: 35
                            Layout.preferredWidth: 200
                            model: [qsTr("Slider tooltip"), qsTr("Label"), qsTr("Hidden")]
                            currentIndex: UserSettings.volumeValueMode
                            onActivated: UserSettings.volumeValueMode = currentIndex
                        }
                    }

                    Item {
                        Layout.fillHeight: true
                    }
                }
            }

            Component {
                id: debugPaneComponent

                ColumnLayout {
                    spacing: 15

                    Label {
                        text: qsTr("Debug and information")
                        font.pixelSize: 22
                        font.bold: true
                    }

                    RowLayout {
                        spacing: 15
                        Layout.preferredHeight: root.rowHeight

                        property int clickCount: 0

                        MouseArea {
                            Layout.fillWidth: true
                            Layout.preferredHeight: root.rowHeight
                            onClicked: {
                                parent.clickCount++
                                if (parent.clickCount >= 5) {
                                    easterEggDialog.open()
                                    parent.clickCount = 0
                                }
                            }

                            RowLayout {
                                anchors.fill: parent
                                spacing: 15

                                Label {
                                    text: qsTr("Application version")
                                    Layout.fillWidth: true
                                }
                                Label {
                                    text: SoundPanelBridge.getAppVersion()
                                    opacity: 0.5
                                }
                            }
                        }
                    }

                    RowLayout {
                        spacing: 15
                        Layout.preferredHeight: root.rowHeight
                        Label {
                            text: qsTr("QT version")
                            Layout.fillWidth: true
                        }

                        Label {
                            text: SoundPanelBridge.getQtVersion()
                            opacity: 0.5
                        }
                    }

                    Item {
                        Layout.fillHeight: true
                    }
                }
            }
        }
    }
}


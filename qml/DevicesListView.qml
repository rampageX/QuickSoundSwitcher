pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.FluentWinUI3
import Odizinne.QuickSoundSwitcher

Rectangle {
    id: root

    property alias model: devicesList.model
    property bool expanded: false
    property real contentOpacity: 0

    // Export the height needed when fully expanded
    property real expandedNeededHeight: devicesList.contentHeight + 20

    // Signals
    signal deviceClicked(string name, string shortName, int index)

    Layout.fillWidth: true
    Layout.preferredHeight: expanded ? expandedNeededHeight : 0
    Layout.leftMargin: -14
    Layout.rightMargin: -14
    color: Constants.footerColor

    Behavior on Layout.preferredHeight {
        NumberAnimation {
            duration: 150
            easing.type: Easing.OutQuad
        }
    }

    // Top border
    Rectangle {
        visible: root.expanded
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        color: Constants.footerBorderColor
        height: 1
        opacity: 0.1
    }

    // Bottom border
    Rectangle {
        visible: root.expanded
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        color: Constants.footerBorderColor
        height: 1
        opacity: 0.1
    }

    ListView {
        id: devicesList
        anchors.fill: parent
        anchors.margins: 10
        clip: true
        interactive: false
        opacity: root.contentOpacity

        Behavior on opacity {
            NumberAnimation {
                duration: 200
                easing.type: Easing.OutQuad
            }
        }

        delegate: ItemDelegate {
            width: devicesList.width
            height: 40
            required property var model
            required property string name
            //required property string shortName
            required property bool isDefault
            required property string deviceId
            required property int index

            highlighted: model.isDefault
            text: UserSettings.deviceShortName ? model.shortName : model.name

            onClicked: {
                // Update the model to mark this device as default
                for (let i = 0; i < devicesList.model.count; i++) {
                    devicesList.model.setProperty(i, "isDefault", i === index)
                }

                // Emit signal to parent
                root.deviceClicked(name, shortName, index)
            }
        }
    }

    Timer {
        id: opacityTimer
        interval: 112
        repeat: false
        onTriggered: root.contentOpacity = 1
    }

    onExpandedChanged: {
        if (expanded) {
            if (UserSettings.opacityAnimations) {
                opacityTimer.start()
            }
            root.contentOpacity = 1
        } else {
            if (UserSettings.opacityAnimations) return
            root.contentOpacity = 0
        }
    }
}

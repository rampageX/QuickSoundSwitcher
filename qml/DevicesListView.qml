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
            required property string description
            required property bool isDefault
            required property string deviceId
            required property int index

            highlighted: model.isDefault

            text: {
                if (UserSettings.deviceShortName) {
                    // Try to create a short name from the full name
                    let fullName = model.name || ""
                    // Simple shortening logic - you can customize this
                    if (fullName.length > 25) {
                        return fullName.substring(0, 22) + "..."
                    }
                    return fullName
                } else {
                    return model.name || ""
                }
            }

            onClicked: {
                // Emit signal to parent with the correct parameters
                root.deviceClicked(model.name, text, index)
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
            } else {
                root.contentOpacity = 1
            }
        } else {
            if (UserSettings.opacityAnimations) {
                root.contentOpacity = 0
            } else {
                root.contentOpacity = 0
            }
        }
    }
}

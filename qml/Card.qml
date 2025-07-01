import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.FluentWinUI3
import Odizinne.QuickSoundSwitcher

Rectangle {
    id: card
    implicitHeight: 70

    property string title: ""
    property string description: ""
    property Item additionalControl

    color: Constants.darkMode ? "#2b2b2b" : "#fbfbfb"
    radius: 5

    onAdditionalControlChanged: {
        if (additionalControl) {
            additionalControl.parent = rowLayout
        }
    }

    Rectangle {
        anchors.fill: parent
        border.width: 1
        color: "transparent"
        border.color: Constants.darkMode ? "#0F0F0F" : "#A0A0A0"
        opacity: 0.5
        radius: 5
    }

    RowLayout {
        id: rowLayout
        spacing: 15
        anchors.fill: parent
        anchors.margins: 15

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            Label {
                text: card.title
                font.pixelSize: 14
                visible: text
            }

            Label {
                text: card.description
                opacity: 0.6
                visible: text
                font.pixelSize: 12
            }
        }

        Item {
            Layout.fillWidth: true
        }
    }
}

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.FluentWinUI3
import Odizinne.QuickSoundSwitcher

ColumnLayout {
    spacing: 3

    Label {
        text: qsTr("Application language")
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

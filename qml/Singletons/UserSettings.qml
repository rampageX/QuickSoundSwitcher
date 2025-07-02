pragma Singleton

import QtCore

Settings {
    property int panelMode: 0
    property bool linkIO: false
    property bool keepAlive: false
    property int panelPosition: 1
    property int taskbarOffset: 52
    property bool deviceShortName: true
    property int volumeValueMode: 0
    property int panelMargin: 12
    property int mediaMode: 0
    property bool displayDevAppLabel: true
    property bool closeDeviceListOnClick: true
    property bool groupApplications: true
    property int languageIndex: 0

    property var commApps: []
    property int chatMixValue: 50
    property bool chatMixEnabled: false
}

pragma Singleton

import QtQuick
import Odizinne.QuickSoundSwitcher


QtObject {
    property bool darkMode: SoundPanelBridge.getDarkMode()
}

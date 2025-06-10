import QtQuick.Controls.Material

ApplicationWindow {
    height: 800
    width: 500
    visible: true
    transientParent: null
    Material.theme: Material.System
    color: Material.theme === Material.Dark ? "#1C1C1C" : "#E3E3E3"
}

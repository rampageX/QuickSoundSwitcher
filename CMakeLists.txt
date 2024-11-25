cmake_minimum_required(VERSION 3.16)
project(QuickSoundSwitcher VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt5 Qt6 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Gui)
find_package(Qt${QT_VERSION_MAJOR} OPTIONAL_COMPONENTS Widgets)

qt_standard_project_setup()

qt_add_executable(QuickSoundSwitcher WIN32
    AudioManager/AudioManager.cpp AudioManager/AudioManager.h
    AudioManager/PolicyConfig.h
    MediaFlyout/MediaFlyout.cpp MediaFlyout/MediaFlyout.h MediaFlyout/MediaFlyout.ui
    MediaSessionWorker/MediaSession.h
    MediaSessionWorker/MediaSessionWorker.cpp MediaSessionWorker/MediaSessionWorker.h
    OverlaySettings/OverlaySettings.cpp OverlaySettings/OverlaySettings.h OverlaySettings/OverlaySettings.ui
    OverlayWidget/OverlayWidget.cpp OverlayWidget/OverlayWidget.h
    Panel/Panel.cpp Panel/Panel.h Panel/Panel.ui
    QuickSoundSwitcher/QuickSoundSwitcher.cpp QuickSoundSwitcher/QuickSoundSwitcher.h
    ShortcutManager/ShortcutManager.cpp ShortcutManager/ShortcutManager.h
    SoundOverlay/SoundOverlay.cpp SoundOverlay/SoundOverlay.h SoundOverlay/SoundOverlay.ui
    Utils/Utils.cpp Utils/Utils.h
    main.cpp
)
target_include_directories(QuickSoundSwitcher PRIVATE
    AudioManager
    MediaFlyout
    MediaSessionWorker
    OverlaySettings
    OverlayWidget
    Panel
    QuickSoundSwitcher
    ShortcutManager
    SoundOverlay
    Utils
)

target_link_libraries(QuickSoundSwitcher PRIVATE
    Qt::Core
    Qt::Gui
    gdi32
    winmm
)


# Resources:
set(resources_resource_files
    "Resources/icons/headset_dark.png"
    "Resources/icons/headset_dark_muted.png"
    "Resources/icons/headset_light.png"
    "Resources/icons/headset_light_muted.png"
    "Resources/icons/mic_dark.png"
    "Resources/icons/mic_dark_muted.png"
    "Resources/icons/mic_light.png"
    "Resources/icons/mic_light_muted.png"
    "Resources/icons/muted_layer.png"
    "Resources/icons/next_dark.png"
    "Resources/icons/next_light.png"
    "Resources/icons/overlay_dark.png"
    "Resources/icons/overlay_light.png"
    "Resources/icons/pause_dark.png"
    "Resources/icons/pause_light.png"
    "Resources/icons/placeholder_dark.png"
    "Resources/icons/placeholder_light.png"
    "Resources/icons/play_dark.png"
    "Resources/icons/play_light.png"
    "Resources/icons/prev_dark.png"
    "Resources/icons/prev_light.png"
    "Resources/icons/tray_dark_0.png"
    "Resources/icons/tray_dark_100.png"
    "Resources/icons/tray_dark_33.png"
    "Resources/icons/tray_dark_66.png"
    "Resources/icons/tray_light_0.png"
    "Resources/icons/tray_light_100.png"
    "Resources/icons/tray_light_33.png"
    "Resources/icons/tray_light_66.png"
    "Resources/icons/w11_icon.png"
)

qt_add_resources(QuickSoundSwitcher "resources"
    PREFIX
        "/"
    BASE
        "Resources"
    FILES
        ${resources_resource_files}
)

if((QT_VERSION_MAJOR GREATER 4))
    target_link_libraries(QuickSoundSwitcher PRIVATE
        Qt::Widgets
    )
endif()

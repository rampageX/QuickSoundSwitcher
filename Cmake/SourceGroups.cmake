source_group("AudioManager" FILES
    AudioManager/AudioManager.cpp
    AudioManager/AudioManager.h
    AudioManager/PolicyConfig.h
)

source_group("MediaFlyout" FILES
    MediaFlyout/MediaFlyout.cpp
    MediaFlyout/MediaFlyout.h
    MediaFlyout/MediaFlyout.ui
)

source_group("MediaSessionWorker" FILES
    MediaSessionWorker/MediaSessionWorker.cpp
    MediaSessionWorker/MediaSessionWorker.h
    MediaSessionWorker/MediaSession.h
)

source_group("OverlaySettings" FILES
    OverlaySettings/OverlaySettings.cpp
    OverlaySettings/OverlaySettings.h
    OverlaySettings/OverlaySettings.ui
)

source_group("OverlayWidget" FILES
    OverlayWidget/OverlayWidget.cpp
    OverlayWidget/OverlayWidget.h
)

source_group("Panel" FILES
    Panel/Panel.cpp
    Panel/Panel.h
    Panel/Panel.ui
)

source_group("QuickSoundSwitcher" FILES
    QuickSoundSwitcher/QuickSoundSwitcher.cpp
    QuickSoundSwitcher/QuickSoundSwitcher.h
)

source_group("ShortcutManager" FILES
    ShortcutManager/ShortcutManager.cpp
    ShortcutManager/ShortcutManager.h
)

source_group("SoundOverlay" FILES
    SoundOverlay/SoundOverlay.cpp
    SoundOverlay/SoundOverlay.h
    SoundOverlay/SoundOverlay.ui
)

source_group("Utils" FILES
    Utils/Utils.cpp
    Utils/Utils.h
)

source_group("Resources" FILES
    ${resources_resource_files}
)

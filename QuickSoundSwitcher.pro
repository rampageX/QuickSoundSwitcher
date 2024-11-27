QT += core gui widgets

TEMPLATE = app
TARGET = QuickSoundSwitcher
CONFIG += c++20 lrelease embed_translations

QM_FILES_RESOURCE_PREFIX=/translations/tr

INCLUDEPATH += \
    AudioManager \
    MediaFlyout \
    MediaSessionWorker \
    OverlaySettings \
    OverlayWidget \
    Panel \
    QuickSoundSwitcher \
    ShortcutManager \
    SoundOverlay \
    Utils

SOURCES += \
    AudioManager/AudioManager.cpp \
    MediaFlyout/MediaFlyout.cpp \
    MediaSessionWorker/MediaSessionWorker.cpp \
    OverlaySettings/OverlaySettings.cpp \
    OverlayWidget/OverlayWidget.cpp \
    Panel/Panel.cpp \
    QuickSoundSwitcher/QuickSoundSwitcher.cpp \
    ShortcutManager/ShortcutManager.cpp \
    SoundOverlay/SoundOverlay.cpp \
    Utils/Utils.cpp \
    main.cpp

HEADERS += \
    AudioManager/AudioManager.h \
    AudioManager/PolicyConfig.h \
    MediaFlyout/MediaFlyout.h \
    MediaSessionWorker/MediaSession.h \
    MediaSessionWorker/MediaSessionWorker.h \
    OverlaySettings/OverlaySettings.h \
    OverlayWidget/OverlayWidget.h \
    Panel/Panel.h \
    QuickSoundSwitcher/QuickSoundSwitcher.h \
    ShortcutManager/ShortcutManager.h \
    SoundOverlay/SoundOverlay.h \
    Utils/Utils.h

FORMS += \
    MediaFlyout/MediaFlyout.ui \
    OverlaySettings/OverlaySettings.ui \
    Panel/Panel.ui \
    SoundOverlay/SoundOverlay.ui

RESOURCES += Resources/resources.qrc

LIBS += -lgdi32 -lwinmm

RC_FILE = Resources/appicon.rc

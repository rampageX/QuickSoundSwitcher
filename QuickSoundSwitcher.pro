QT += core gui widgets qml quick

TEMPLATE = app
TARGET = QuickSoundSwitcher
CONFIG += c++20 lrelease embed_translations silent

QM_FILES_RESOURCE_PREFIX=/translations/tr

INCLUDEPATH += \
    AudioManager \
    MediaFlyout \
    MediaSessionWorker \
    SettingsPage \
    Panel \
    QuickSoundSwitcher \
    ShortcutManager \
    SoundOverlay \
    Utils

SOURCES += \
    AudioManager/AudioManager.cpp \
    MediaFlyout/MediaFlyout.cpp \
    MediaSessionWorker/MediaSessionWorker.cpp \
    SettingsPage/SettingsPage.cpp \
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
    SettingsPage/SettingsPage.h \
    Panel/Panel.h \
    QuickSoundSwitcher/QuickSoundSwitcher.h \
    ShortcutManager/ShortcutManager.h \
    SoundOverlay/SoundOverlay.h \
    Utils/Utils.h

FORMS += \
    MediaFlyout/MediaFlyout.ui \
    SettingsPage/SettingsPage.ui \
    Panel/Panel.ui \

RESOURCES += Resources/resources.qrc

LIBS += -lgdi32 -lwinmm

RC_FILE = Resources/appicon.rc

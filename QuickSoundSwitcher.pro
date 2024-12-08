QT += core gui widgets qml quick

TEMPLATE = app
TARGET = QuickSoundSwitcher
CONFIG += c++20 lrelease embed_translations silent

QM_FILES_RESOURCE_PREFIX=/translations/tr

INCLUDEPATH += \
    AudioManager \
    MediaFlyout \
    QuickSoundSwitcher \
    ShortcutManager \
    Utils

SOURCES += \
    AudioManager/AudioManager.cpp \
    MediaFlyout/MediaFlyout.cpp \
    QuickSoundSwitcher/QuickSoundSwitcher.cpp \
    ShortcutManager/ShortcutManager.cpp \
    Utils/Utils.cpp \
    main.cpp

HEADERS += \
    AudioManager/AudioManager.h \
    AudioManager/PolicyConfig.h \
    MediaFlyout/MediaFlyout.h \
    QuickSoundSwitcher/QuickSoundSwitcher.h \
    ShortcutManager/ShortcutManager.h \
    Utils/Utils.h

RESOURCES += Resources/resources.qrc

LIBS += -lgdi32 -lwinmm

RC_FILE = Resources/appicon.rc

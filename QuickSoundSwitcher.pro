QT += core gui widgets qml quick

TEMPLATE = app
TARGET = QuickSoundSwitcher
CONFIG += c++20 lrelease embed_translations silent

QM_FILES_RESOURCE_PREFIX=/translations/tr

INCLUDEPATH += \
    AudioManager \
    SoundPanel \
    QuickSoundSwitcher \
    ShortcutManager \
    Utils \
    Dependencies

SOURCES += \
    AudioManager/AudioManager.cpp \
    SoundPanel/SoundPanel.cpp \
    QuickSoundSwitcher/QuickSoundSwitcher.cpp \
    ShortcutManager/ShortcutManager.cpp \
    Utils/Utils.cpp \
    main.cpp

HEADERS += \
    AudioManager/AudioManager.h \
    AudioManager/PolicyConfig.h \
    SoundPanel/SoundPanel.h \
    QuickSoundSwitcher/QuickSoundSwitcher.h \
    ShortcutManager/ShortcutManager.h \
    Utils/Utils.h \
    Dependencies/qstylehelper.hpp

RESOURCES += Resources/resources.qrc

LIBS += -lgdi32 -lwinmm

RC_FILE = Resources/appicon.rc

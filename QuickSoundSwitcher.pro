QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

INCLUDEPATH +=                                      \
    QuickSoundSwitcher/                             \
    Utils/                                          \
    Panel/                                          \
    AudioManager/                                   \
    ShortcutManager/                                \

SOURCES +=                                          \
    main.cpp                                        \
    Panel/Panel.cpp                                 \
    QuickSoundSwitcher/QuickSoundSwitcher.cpp       \
    Utils/Utils.cpp                                 \
    AudioManager/AudioManager.cpp                   \
    ShortcutManager/ShortcutManager.cpp             \

HEADERS +=                                          \
    Panel/Panel.h                                   \
    QuickSoundSwitcher/QuickSoundSwitcher.h         \
    Utils/Utils.h                                   \
    AudioManager/AudioManager.h                     \
    AudioManager/PolicyConfig.h                     \
    ShortcutManager/ShortcutManager.h               \

FORMS +=                                            \
    Panel/Panel.ui                                  \

RESOURCES +=                                        \
    Resources/resources.qrc                         \

LIBS += -luser32 -ladvapi32 -lwinmm -lole32 -lgdi32

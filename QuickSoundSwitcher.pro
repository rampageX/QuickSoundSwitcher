QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += \
    src/QuickSoundSwitcher/ \
    src/Utils/ \
    src/Panel/ \
    src/AudioManager/

SOURCES += \
    src/main.cpp \
    src/Panel/panel.cpp \
    src/QuickSoundSwitcher/quicksoundswitcher.cpp \
    src/Utils/utils.cpp \
    src/AudioManager/audiomanager.cpp

HEADERS += \
    customtrayicon.h \
    src/Panel/panel.h \
    src/QuickSoundSwitcher/quicksoundswitcher.h \
    src/Utils/utils.h \
    src/AudioManager/audiomanager.h

RESOURCES += \
    src/Resources/resources.qrc \

LIBS += -luser32 -ladvapi32 -lwinmm

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

FORMS += \
    src/Panel/panel.ui

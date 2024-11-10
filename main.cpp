#include "QuickSoundSwitcher.h"
#include <QApplication>
#include <QMessageBox>
#include <QProcess>
#include <QTranslator>
#include <QLocale>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyle("fusion");
    a.setQuitOnLastWindowClosed(false);

    QLocale locale;
    QString languageCode = locale.name().section('_', 0, 0);
    QTranslator translator;

    if (translator.load(":/translations/QuickSoundSwitcher_" + languageCode + ".qm")) {
        a.installTranslator(&translator);
    }

    QuickSoundSwitcher w;

    return a.exec();
}

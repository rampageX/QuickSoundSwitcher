#include "QuickSoundSwitcher.h"
#include "Utils.h"
#include <QApplication>
#include <QTranslator>
#include <QLocale>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyle("fusion");
    a.setQuitOnLastWindowClosed(false);

    QString mode = Utils::getTheme();
    QString color;
    QPalette palette = QApplication::palette();

    if (mode == "light") {
        color = Utils::getAccentColor("light1");
    } else {
        color = Utils::getAccentColor("dark1");
    }

    QColor highlightColor(color);

    palette.setColor(QPalette::Highlight, highlightColor);
    a.setPalette(palette);

    QLocale locale;
    QString languageCode = locale.name().section('_', 0, 0);
    QTranslator translator;

    if (translator.load(":/translations/QuickSoundSwitcher_" + languageCode + ".qm")) {
        a.installTranslator(&translator);
    }

    QuickSoundSwitcher w;

    return a.exec();
}

#include "quicksoundswitcher.h"
#include <QApplication>
#include <QMessageBox>
#include <QProcess>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QuickSoundSwitcher w;
    a.setStyle("fusion");
    a.setQuitOnLastWindowClosed(false);
    return a.exec();
}

#ifndef QUICKSOUNDSWITCHER_H
#define QUICKSOUNDSWITCHER_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include "panel.h"

class QuickSoundSwitcher : public QMainWindow
{
    Q_OBJECT

public:
    QuickSoundSwitcher(QWidget *parent = nullptr);
    ~QuickSoundSwitcher();

private slots:
    void onVolumeChanged();
    void onPanelClosed();

private:
    QSystemTrayIcon *trayIcon;
    Panel* panel;
    void createTrayIcon();
    void showPanel();
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
};

#endif // QUICKSOUNDSWITCHER_H

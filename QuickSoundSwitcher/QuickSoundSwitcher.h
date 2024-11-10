#ifndef QUICKSOUNDSWITCHER_H
#define QUICKSOUNDSWITCHER_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include "Panel.h"

class QuickSoundSwitcher : public QMainWindow
{
    Q_OBJECT

public:
    QuickSoundSwitcher(QWidget *parent = nullptr);
    ~QuickSoundSwitcher();

private slots:
    void onVolumeChanged();
    void onOutputMuteChanged();
    void onPanelClosed();
    void onRunAtStartupStateChanged();

private:
    QSystemTrayIcon *trayIcon;
    Panel* panel;
    int currentY;
    int targetY;
    void createTrayIcon();
    void showPanel();
    void hidePanel();
    void movePanelDown();
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
    bool hiding;
};

#endif // QUICKSOUNDSWITCHER_H

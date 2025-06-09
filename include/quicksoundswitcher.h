#ifndef QUICKSOUNDSWITCHER_H
#define QUICKSOUNDSWITCHER_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QSettings>
#include <QQmlApplicationEngine>
#include <QWindow>
#include <Windows.h>

class QuickSoundSwitcher : public QWidget
{
    Q_OBJECT

public:
    QuickSoundSwitcher(QWidget *parent = nullptr);
    ~QuickSoundSwitcher();
    static QuickSoundSwitcher* instance;
    void adjustOutputVolume(bool up);
    void toggleMuteWithKey();

private slots:
    void onRunAtStartupStateChanged();
    void onOutputMuteChanged();
    void onMixerOnlyStateChanged();
    void onLinkIOStateChanged();
    void onPanelHideAnimationFinished();
    void setWindowTopmost();
    void setWindowNotTopmost();

private:
    QSystemTrayIcon *trayIcon;
    QQmlApplicationEngine* engine;
    QWindow* panelWindow;
    bool isPanelVisible;
    QSettings settings;

    void createQMLEngine();
    void destroyQMLEngine();
    void createTrayIcon();
    void togglePanel();
    void showPanel();
    void hidePanel();
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);

    static LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
    static HHOOK mouseHook;
    static HHOOK keyboardHook;

    void installGlobalMouseHook();
    void uninstallGlobalMouseHook();
    void installKeyboardHook();
    void uninstallKeyboardHook();
};

#endif // QUICKSOUNDSWITCHER_H

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
    void onOutputMuteChanged();
    void onPanelHideAnimationFinished();
    void onDataInitializationComplete();
    void onLanguageChanged();

private:
    QSystemTrayIcon *trayIcon;
    QQmlApplicationEngine* engine;
    QWindow* panelWindow;
    bool isPanelVisible;
    QSettings settings;
    QAction *outputDeviceAction;
    QAction *inputDeviceAction;
    QString currentOutputDevice;
    QString currentInputDevice;

    void initializeQMLEngine();
    void destroyQMLEngine();
    void createTrayIcon();
    void togglePanel();
    void showPanel();
    void hidePanel();
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void initializeTrayMenuDeviceInfo();
    void updateTrayMenuDeviceInfo();
    QString elideDeviceText(const QString& deviceName, int volume, bool muted);

    static LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
    static HHOOK mouseHook;
    static HHOOK keyboardHook;

    void installGlobalMouseHook();
    void uninstallGlobalMouseHook();
    void installKeyboardHook();
    void uninstallKeyboardHook();

    void updateTrayMenu();
};

#endif // QUICKSOUNDSWITCHER_H

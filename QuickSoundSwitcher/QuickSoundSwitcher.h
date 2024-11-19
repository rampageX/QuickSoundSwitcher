#ifndef QUICKSOUNDSWITCHER_H
#define QUICKSOUNDSWITCHER_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include "Panel.h"
#include <QSettings>
#include "OverlaySettings.h"
#include "OverlayWidget.h"

class QuickSoundSwitcher : public QMainWindow
{
    Q_OBJECT

public:
    QuickSoundSwitcher(QWidget *parent = nullptr);
    ~QuickSoundSwitcher();
    static QuickSoundSwitcher* instance;
    void adjustOutputVolume(bool up);

private slots:
    void onVolumeChanged();
    void onOutputMuteChanged();
    void onInputMuteChanged();
    void onPanelClosed();
    void onRunAtStartupStateChanged();

protected:
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;

private:
    QSystemTrayIcon *trayIcon;
    Panel* panel;
    void createTrayIcon();
    void showPanel();
    void hidePanel();
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
    bool hiding;
    void createDeviceSubMenu(QMenu *parentMenu, const QList<AudioDevice> &devices, const QString &title);

    static LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam);
    static HHOOK mouseHook;
    void installGlobalMouseHook();
    void uninstallGlobalMouseHook();

    static const int HOTKEY_ID = 1;
    bool registerGlobalHotkey();
    void unregisterGlobalHotkey();
    void toggleMicMute();
    void toggleMutedOverlay(bool enabled);
    void sendNotification(bool enabled);
    void loadSettings();
    void onSettingsChanged();
    void onSettingsClosed();
    void showSettings();
    bool disableNotification;
    bool disableOverlay;
    bool potatoMode;
    OverlayWidget *overlayWidget;
    OverlaySettings *overlaySettings;
    QSettings settings;
    QString position;
    bool isMuted;

signals:
    void muteStateChanged();
    void volumeChangedWithTray();
};

#endif // QUICKSOUNDSWITCHER_H

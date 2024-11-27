#ifndef QUICKSOUNDSWITCHER_H
#define QUICKSOUNDSWITCHER_H

#include "MediaFlyout.h"
#include "MediaSessionWorker.h"
#include "Panel.h"
#include "SettingsPage.h"
#include "SoundOverlay.h"
#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QSettings>

class QuickSoundSwitcher : public QMainWindow
{
    Q_OBJECT

public:
    QuickSoundSwitcher(QWidget *parent = nullptr);
    ~QuickSoundSwitcher();
    static QuickSoundSwitcher* instance;
    void adjustOutputVolume(bool up);
    void toggleMuteWithKey();

public slots:
    void onVolumeChanged();
    void onOutputMuteChanged();

private slots:
    void onPanelClosed();
    void onSoundOverlayClosed();
    void onRunAtStartupStateChanged();
    void onRequestNext();
    void onRequestPrev();
    void onRequestPause();
    void onSessionReady(const MediaSession& session);
    void onSessionError();
    void updateFlyoutTitleAndArtist(const QString& title, const QString& artist);
    void updateFlyoutIcon(QIcon icon);
    void updateFlyoutPlayPause(const QString& state);
    void updateFlyoutProgress(int currentTime, int totalTime);

protected:
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
    bool event(QEvent *event) override;

private:
    QSystemTrayIcon *trayIcon;
    QSettings settings;
    Panel* panel;
    MediaFlyout* mediaFlyout;
    SoundOverlay* soundOverlay;
    void createTrayIcon();
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

    static const int HOTKEY_ID = 1;
    void toggleMicMute();
    void loadSettings();
    void onSettingsChanged();
    void onSettingsClosed();
    void showSettings();
    void updateApplicationColorScheme();
    bool disableNotification;
    bool disableOverlay;
    bool potatoMode;
    bool hotkeyRegistered;
    bool disableMuteHotkey;
    bool mergeSimilarApps;
    int volumeIncrement;
    SettingsPage *settingsPage;
    QString position;

    QThread* workerThread;
    MediaSessionWorker* worker;
    QTimer* mediaSessionTimer;
    void stopMonitoringMediaSession();
    void getMediaSession();
    bool currentlyPlaying;

signals:
    void muteStateChanged();
    void outputMuteStateChanged(bool state);
    void volumeChangedWithTray();
};

#endif // QUICKSOUNDSWITCHER_H

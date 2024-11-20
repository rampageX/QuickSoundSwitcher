#include "QuickSoundSwitcher.h"
#include "Utils.h"
#include "AudioManager.h"
#include "ShortcutManager.h"
#include <QMenu>
#include <QApplication>
#include <QScreen>
#include <QRect>
#include <QTimer>
#include <windows.h>
#include <QThread>

HHOOK QuickSoundSwitcher::keyboardHook = NULL;
HHOOK QuickSoundSwitcher::mouseHook = NULL;
QuickSoundSwitcher* QuickSoundSwitcher::instance = nullptr;

QuickSoundSwitcher::QuickSoundSwitcher(QWidget *parent)
    : QMainWindow(parent)
    , trayIcon(new QSystemTrayIcon(this))
    , panel(nullptr)
    , hiding(false)
    , overlayWidget(nullptr)
    , overlaySettings(nullptr)
    , settings("Odizinne", "QuickSoundSwitcher")
{
    instance = this;
    createTrayIcon();
    registerGlobalHotkey();
    loadSettings();
    toggleMutedOverlay(AudioManager::getRecordingMute());
    installGlobalMouseHook();
    installKeyboardHook();
}

QuickSoundSwitcher::~QuickSoundSwitcher()
{
    unregisterGlobalHotkey();
    uninstallGlobalMouseHook();
    uninstallKeyboardHook();
    instance = nullptr;
}

void QuickSoundSwitcher::createTrayIcon()
{
    onOutputMuteChanged();

    QMenu *trayMenu = new QMenu(this);

    connect(trayMenu, &QMenu::aboutToShow, this, [trayMenu, this]() {
        trayMenu->clear();

        QList<AudioDevice> playbackDevices;
        AudioManager::enumeratePlaybackDevices(playbackDevices);

        QList<AudioDevice> recordingDevices;
        AudioManager::enumerateRecordingDevices(recordingDevices);

        QAction *startupAction = new QAction(tr("Run at startup"), this);
        startupAction->setCheckable(true);
        startupAction->setChecked(ShortcutManager::isShortcutPresent());
        connect(startupAction, &QAction::triggered, this, &QuickSoundSwitcher::onRunAtStartupStateChanged);

        QAction *settingsAction = new QAction(tr("Mute overlay settings"), this);
        connect(settingsAction, &QAction::triggered, this, &QuickSoundSwitcher::showSettings);

        QAction *exitAction = new QAction(tr("Exit"), this);
        connect(exitAction, &QAction::triggered, this, &QApplication::quit);

        createDeviceSubMenu(trayMenu, playbackDevices, tr("Output device"));
        createDeviceSubMenu(trayMenu, recordingDevices, tr("Input device"));
        trayMenu->addSeparator();
        trayMenu->addAction(startupAction);
        trayMenu->addAction(settingsAction);
        trayMenu->addSeparator();
        trayMenu->addAction(exitAction);
    });

    trayIcon->setContextMenu(trayMenu);
    trayIcon->setToolTip("Quick Sound Switcher");
    trayIcon->show();
    connect(trayIcon, &QSystemTrayIcon::activated, this, &QuickSoundSwitcher::trayIconActivated);
}

void QuickSoundSwitcher::createDeviceSubMenu(QMenu *parentMenu, const QList<AudioDevice> &devices, const QString &title)
{
    QMenu *subMenu = parentMenu->addMenu(title);

    for (const AudioDevice &device : devices) {
        QAction *deviceAction = new QAction(device.shortName, subMenu);
        deviceAction->setCheckable(true);
        deviceAction->setChecked(device.isDefault);

        connect(deviceAction, &QAction::triggered, this, [device]() {
            AudioManager::setDefaultEndpoint(device.id);
        });

        subMenu->addAction(deviceAction);
    }
}

void QuickSoundSwitcher::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger) {
        showPanel();
    }
}

void QuickSoundSwitcher::showPanel()
{
    if (hiding) {
        return;
    }
    if (panel) {
        hidePanel();
        return;
    }

    panel = new Panel(this);

    connect(panel, &Panel::volumeChanged, this, &QuickSoundSwitcher::onVolumeChanged);
    connect(panel, &Panel::outputMuteChanged, this, &QuickSoundSwitcher::onOutputMuteChanged);
    connect(panel, &Panel::inputMuteChanged, this, &QuickSoundSwitcher::onInputMuteChanged);
    connect(panel, &Panel::lostFocus, this, &QuickSoundSwitcher::hidePanel);
    connect(panel, &Panel::panelClosed, this, &QuickSoundSwitcher::onPanelClosed);

    panel->animateIn(trayIcon->geometry());
}

void QuickSoundSwitcher::hidePanel()
{
    if (!panel) return;
    hiding = true;

    panel->animateOut(trayIcon->geometry());
}

void QuickSoundSwitcher::onPanelClosed()
{
    delete panel;
    panel = nullptr;
    hiding = false;
}

void QuickSoundSwitcher::onVolumeChanged()
{
    trayIcon->setIcon(Utils::getIcon(1, AudioManager::getPlaybackVolume(), NULL));
}

void QuickSoundSwitcher::onOutputMuteChanged()
{
    int volumeIcon;
    if (AudioManager::getPlaybackMute()) {
        volumeIcon = 0;
    } else {
        volumeIcon = AudioManager::getPlaybackVolume();
    }

    trayIcon->setIcon(Utils::getIcon(1, volumeIcon, NULL));
}

void QuickSoundSwitcher::onInputMuteChanged()
{
    toggleMutedOverlay(AudioManager::getRecordingMute());
    sendNotification(AudioManager::getRecordingMute());
}

void QuickSoundSwitcher::onRunAtStartupStateChanged()
{
    QAction *action = qobject_cast<QAction *>(sender());
    ShortcutManager::manageShortcut(action->isChecked());
}

bool QuickSoundSwitcher::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
    MSG *msg = static_cast<MSG *>(message);
    if (msg->message == WM_HOTKEY) {
        if (msg->wParam == HOTKEY_ID) {
            toggleMicMute();
            return true;
        }
    }
    return QWidget::nativeEvent(eventType, message, result);
}

bool QuickSoundSwitcher::registerGlobalHotkey() {
    return RegisterHotKey((HWND)this->winId(), HOTKEY_ID, MOD_CONTROL | MOD_ALT, 0x4D);
}

void QuickSoundSwitcher::unregisterGlobalHotkey() {
    UnregisterHotKey((HWND)this->winId(), HOTKEY_ID);
}

void QuickSoundSwitcher::toggleMicMute()
{
    AudioManager::setRecordingMute(!AudioManager::getRecordingMute());

    if (AudioManager::getRecordingMute()) {
        sendNotification(true);
        toggleMutedOverlay(true);
    } else {
        sendNotification(false);
        toggleMutedOverlay(false);
    }

    if (panel) {
        emit muteStateChanged();
    }
}

void QuickSoundSwitcher::toggleMutedOverlay(bool enabled)
{
    if (disableOverlay) {
        if (overlayWidget != nullptr) {
            overlayWidget->hide();
            delete overlayWidget;
            overlayWidget = nullptr;
        }
        return;
    }

    if (enabled) {
        if (overlayWidget == nullptr) {
            overlayWidget = new OverlayWidget(position, potatoMode, this);
        }
        overlayWidget->show();
    } else {
        if (overlayWidget != nullptr) {
            overlayWidget->hide();
        }
    }
}

void QuickSoundSwitcher::sendNotification(bool enabled)
{
    if (disableNotification) {
        return;
    }

    if (enabled) {
        Utils::playSoundNotification(false);
    } else {
        Utils::playSoundNotification(true);
    }
}

void QuickSoundSwitcher::showSettings()
{
    if (overlaySettings) {
        overlaySettings->showNormal();
        overlaySettings->raise();
        overlaySettings->activateWindow();
        return;
    }

    overlaySettings = new OverlaySettings;
    overlaySettings->setAttribute(Qt::WA_DeleteOnClose);
    connect(overlaySettings, &OverlaySettings::closed, this, &QuickSoundSwitcher::onSettingsClosed);
    connect(overlaySettings, &OverlaySettings::settingsChanged, this, &QuickSoundSwitcher::onSettingsChanged);
    overlaySettings->show();
}

void QuickSoundSwitcher::loadSettings()
{
    position = settings.value("overlayPosition", "topRightCorner").toString();
    disableOverlay = settings.value("disableOverlay", true).toBool();
    potatoMode = settings.value("potatoMode", false).toBool();
    disableNotification = settings.value("disableNotification", true).toBool();
}

void QuickSoundSwitcher::onSettingsChanged()
{
    loadSettings();

    if (AudioManager::getRecordingMute()) {
        toggleMutedOverlay(!disableOverlay);
    }

    if (!disableOverlay) {
        if (!overlayWidget) {
            overlayWidget = new OverlayWidget(position, potatoMode, this);
        }
        overlayWidget->moveOverlayToPosition(position);
    }
}

void QuickSoundSwitcher::onSettingsClosed()
{
    disconnect(overlaySettings, &OverlaySettings::closed, this, &QuickSoundSwitcher::onSettingsClosed);
    disconnect(overlaySettings, &OverlaySettings::settingsChanged, this, &QuickSoundSwitcher::onSettingsChanged);
    overlaySettings = nullptr;
}

void QuickSoundSwitcher::installGlobalMouseHook()
{
    if (mouseHook == NULL) {
        mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, NULL, 0);
    }
}

void QuickSoundSwitcher::uninstallGlobalMouseHook()
{
    if (mouseHook != NULL) {
        UnhookWindowsHookEx(mouseHook);
        mouseHook = NULL;
    }
}

void QuickSoundSwitcher::installKeyboardHook()
{
    if (keyboardHook == NULL) {
        keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
    }
}

void QuickSoundSwitcher::uninstallKeyboardHook()
{
    if (keyboardHook != NULL) {
        UnhookWindowsHookEx(keyboardHook);
        keyboardHook = NULL;
    }
}

LRESULT CALLBACK QuickSoundSwitcher::MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        MSLLHOOKSTRUCT *hookStruct = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);
        if (wParam == WM_MOUSEWHEEL) {
            int zDelta = GET_WHEEL_DELTA_WPARAM(hookStruct->mouseData);  // Check the scroll direction
            QPoint cursorPos = QCursor::pos();  // Get current cursor position

            QRect trayIconRect = instance->trayIcon->geometry();  // Get tray icon position
            if (trayIconRect.contains(cursorPos)) {  // Check if the cursor is over the tray icon
                if (zDelta > 0) {
                    instance->adjustOutputVolume(true);
                } else {
                    instance->adjustOutputVolume(false);
                }
            }
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT CALLBACK QuickSoundSwitcher::KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION) {
        PKBDLLHOOKSTRUCT pKeyboard = reinterpret_cast<PKBDLLHOOKSTRUCT>(lParam);

        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            switch (pKeyboard->vkCode) {
            case VK_VOLUME_UP:
                instance->adjustOutputVolume(true);
                return 1;
            case VK_VOLUME_DOWN:
                instance->adjustOutputVolume(false);
                return 1;
            case VK_VOLUME_MUTE:
                instance->toggleMuteWithKey();
                return 1;
            default:
                break;
            }
        }
    }

    return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
}

void QuickSoundSwitcher::adjustOutputVolume(bool up)
{
    int volume = AudioManager::getPlaybackVolume();
    if (up) {
        int newVolume = volume + 2;
        if (newVolume > 100) {
            newVolume = 100;
        }
        AudioManager::setPlaybackVolume(newVolume);
    } else {
        int newVolume = volume - 2;
        if (newVolume < 0) {
            newVolume = 0;
        }
        AudioManager::setPlaybackVolume(newVolume);
    }

    trayIcon->setIcon(Utils::getIcon(1, AudioManager::getPlaybackVolume(), NULL));
    emit volumeChangedWithTray();
}

void QuickSoundSwitcher::toggleMuteWithKey()
{
    bool newPlaybackMute = !AudioManager::getPlaybackMute();
    AudioManager::setPlaybackMute(newPlaybackMute);
    onOutputMuteChanged();
    emit outputMuteStateChanged(newPlaybackMute);
}

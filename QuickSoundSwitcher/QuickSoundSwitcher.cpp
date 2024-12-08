#include "QuickSoundSwitcher.h"
#include "Utils.h"
#include "AudioManager.h"
#include "ShortcutManager.h"
#include <QMenu>
#include <QApplication>
#include <QScreen>
#include <QRect>
#include <Windows.h>

HHOOK QuickSoundSwitcher::keyboardHook = NULL;
HHOOK QuickSoundSwitcher::mouseHook = NULL;
QuickSoundSwitcher* QuickSoundSwitcher::instance = nullptr;

QuickSoundSwitcher::QuickSoundSwitcher(QWidget *parent)
    : QMainWindow(parent)
    , trayIcon(new QSystemTrayIcon(this))
    , soundPanel(nullptr)
{
    AudioManager::initialize();
    instance = this;
    createTrayIcon();
    installGlobalMouseHook();
    installKeyboardHook();

}

QuickSoundSwitcher::~QuickSoundSwitcher()
{
    AudioManager::cleanup();
    uninstallGlobalMouseHook();
    uninstallKeyboardHook();
    delete soundPanel;
    instance = nullptr;
}

void QuickSoundSwitcher::createTrayIcon()
{
    onOutputMuteChanged();

    QMenu *trayMenu = new QMenu(this);

    QAction *startupAction = new QAction(tr("Run at startup"), this);
    startupAction->setCheckable(true);
    startupAction->setChecked(ShortcutManager::isShortcutPresent("QuickSoundSwitcher.lnk"));
    connect(startupAction, &QAction::triggered, this, &QuickSoundSwitcher::onRunAtStartupStateChanged);

    QAction *exitAction = new QAction(tr("Exit"), this);
    connect(exitAction, &QAction::triggered, this, &QApplication::quit);

    trayMenu->addAction(startupAction);
    trayMenu->addAction(exitAction);

    trayIcon->setContextMenu(trayMenu);
    trayIcon->setToolTip("Quick Sound Switcher");
    trayIcon->show();
    connect(trayIcon, &QSystemTrayIcon::activated, this, &QuickSoundSwitcher::trayIconActivated);
}

bool QuickSoundSwitcher::event(QEvent *event)
{
    if (event->type() == QEvent::ApplicationPaletteChange) {
        onOutputMuteChanged();
        return true;
    }
    return QMainWindow::event(event);
}

void QuickSoundSwitcher::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger) {
        togglePanel();
    }
}

void QuickSoundSwitcher::onOutputMuteChanged()
{
    int volumeIcon;
    if (AudioManager::getPlaybackMute()) {
        volumeIcon = 0;
    } else {
        volumeIcon = AudioManager::getPlaybackVolume();
    }

    trayIcon->setIcon(QIcon(Utils::getIcon(1, volumeIcon, NULL)));
}

void QuickSoundSwitcher::onRunAtStartupStateChanged()
{
    QAction *action = qobject_cast<QAction *>(sender());
    ShortcutManager::manageShortcut(action->isChecked(), "QuickSoundSwitcher.lnk");
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
            int zDelta = GET_WHEEL_DELTA_WPARAM(hookStruct->mouseData);
            QPoint cursorPos = QCursor::pos();

            QRect trayIconRect = instance->trayIcon->geometry();
            if (trayIconRect.contains(cursorPos)) {
                if (zDelta > 0) {
                    instance->adjustOutputVolume(true);
                } else {
                    instance->adjustOutputVolume(false);
                }
            }
        }

        if (wParam == WM_LBUTTONUP || wParam == WM_RBUTTONUP) {
            QPoint cursorPos = QCursor::pos();
            QRect trayIconRect = instance->trayIcon->geometry();
            QRect soundPanelRect;

            if (instance->soundPanel && instance->soundPanel->soundPanelWindow) {
                soundPanelRect = instance->soundPanel->soundPanelWindow->geometry();
            } else {
                soundPanelRect = QRect();
            }

            if (!soundPanelRect.contains(cursorPos) && !trayIconRect.contains(cursorPos)) {
                if (instance->soundPanel) {
                    delete instance->soundPanel;
                    instance->soundPanel = nullptr;
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
                break;
            case VK_VOLUME_DOWN:
                instance->adjustOutputVolume(false);
                break;
            case VK_VOLUME_MUTE:
                instance->toggleMuteWithKey();
                break;
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
        volume = volume + 2;
    } else {
        volume = volume - 2;
    }
    trayIcon->setIcon(QIcon(Utils::getIcon(1, volume, NULL)));
}

void QuickSoundSwitcher::toggleMuteWithKey()
{
    bool muted = !AudioManager::getPlaybackMute();
    int volume = AudioManager::getPlaybackVolume();

    int volumeIcon;
    if (volume == 0 || muted) {
        volumeIcon = 0;
    } else {
        volumeIcon = volume;
    }

    trayIcon->setIcon(QIcon(Utils::getIcon(1, volumeIcon, NULL)));
}

void QuickSoundSwitcher::togglePanel()
{
    if (!soundPanel) {
        soundPanel = new SoundPanel(this);
        connect(soundPanel, &SoundPanel::shouldUpdateTray, this, &QuickSoundSwitcher::onOutputMuteChanged);
    } else {
        delete soundPanel;
        soundPanel = nullptr;
    }
}

#include "quicksoundswitcher.h"
#include "soundpanelbridge.h"
#include "utils.h"
#include "audiomanager.h"
#include "shortcutmanager.h"
#include <QMenu>
#include <QApplication>
#include <QScreen>
#include <QRect>
#include <QWindow>
#include <QQmlContext>
#include <QTimer>
#include <Windows.h>

HHOOK QuickSoundSwitcher::keyboardHook = NULL;
HHOOK QuickSoundSwitcher::mouseHook = NULL;
QuickSoundSwitcher* QuickSoundSwitcher::instance = nullptr;
static bool validMouseDown = false;

QuickSoundSwitcher::QuickSoundSwitcher(QWidget *parent)
    : QWidget(parent)
    , trayIcon(new QSystemTrayIcon(this))
    , engine(nullptr)
    , settingsEngine(new QQmlApplicationEngine)
    , panelWindow(nullptr)
    , isPanelVisible(false)
    , settings("Odizinne", "QuickSoundSwitcher")
{
    AudioManager::initialize();
    instance = this;
    createTrayIcon();
    installKeyboardHook();

    bool firstRun = settings.value("firstRun", true).toBool();
    if (firstRun) {
        settings.setValue("firstRun", false);
        trayIcon->showMessage(
            "Access sound panel from the system tray",
            "This notification won't show again",
            QSystemTrayIcon::NoIcon
            );
    }


}

QuickSoundSwitcher::~QuickSoundSwitcher()
{
    AudioManager::cleanup();
    uninstallGlobalMouseHook();
    uninstallKeyboardHook();
    destroyQMLEngine();
    instance = nullptr;
}

void QuickSoundSwitcher::setWindowTopmost()
{
    if (panelWindow) {
        HWND hWnd = reinterpret_cast<HWND>(panelWindow->winId());
        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
}

void QuickSoundSwitcher::setWindowNotTopmost()
{
    if (panelWindow) {
        HWND hWnd = reinterpret_cast<HWND>(panelWindow->winId());
        SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
}

void QuickSoundSwitcher::createQMLEngine()
{
    if (engine) {
        destroyQMLEngine();
    }

    engine = new QQmlApplicationEngine(this);
    engine->loadFromModule("Odizinne.QuickSoundSwitcher", "SoundPanel");

    if (!engine->rootObjects().isEmpty()) {
        panelWindow = qobject_cast<QWindow*>(engine->rootObjects().first());
        if (panelWindow) {
            panelWindow->setProperty("visible", false);

            // Connect to animation signals
            connect(panelWindow, SIGNAL(hideAnimationFinished()),
                    this, SLOT(onPanelHideAnimationFinished()));
            connect(panelWindow, SIGNAL(showAnimationFinished()),
                    this, SLOT(setWindowTopmost()));
            connect(panelWindow, SIGNAL(hideAnimationStarted()),
                    this, SLOT(setWindowNotTopmost()));
        }
    }

    if (SoundPanelBridge::instance()) {
        connect(SoundPanelBridge::instance(), &SoundPanelBridge::shouldUpdateTray,
                this, &QuickSoundSwitcher::onOutputMuteChanged);
    }
}

void QuickSoundSwitcher::destroyQMLEngine()
{
    if (engine) {
        engine->deleteLater();
        engine = nullptr;
    }
    panelWindow = nullptr;
}

void QuickSoundSwitcher::createTrayIcon()
{
    onOutputMuteChanged();

    if (settings.value("mixerOnly").toBool()) {
        QString theme = Utils::getTheme();
        if (theme == "light") {
            trayIcon->setIcon(QIcon(":/icons/system_light.png"));
        } else {
            trayIcon->setIcon(QIcon(":/icons/system_dark.png"));
        }
    }

    QMenu *trayMenu = new QMenu(this);

    QAction *settingsaction = new QAction(tr("Settings"), this);
    connect(settingsaction, &QAction::triggered, this, &QuickSoundSwitcher::onSettingsActionActivated);

    QAction *mixerOnly = new QAction(tr("Use volume mixer only"), this);
    mixerOnly->setCheckable(true);
    mixerOnly->setChecked(settings.value("mixerOnly", false).toBool());
    connect(mixerOnly, &QAction::triggered, this, &QuickSoundSwitcher::onMixerOnlyStateChanged);

    QAction *linkIO = new QAction(tr("Link Input / Output devices"), this);
    linkIO->setCheckable(true);
    linkIO->setChecked(settings.value("linkIO", false).toBool());
    connect(linkIO, &QAction::triggered, this, &QuickSoundSwitcher::onLinkIOStateChanged);

    QAction *startupAction = new QAction(tr("Run at startup"), this);
    startupAction->setCheckable(true);
    startupAction->setChecked(ShortcutManager::isShortcutPresent("QuickSoundSwitcher.lnk"));
    connect(startupAction, &QAction::triggered, this, &QuickSoundSwitcher::onRunAtStartupStateChanged);

    QAction *exitAction = new QAction(tr("Exit"), this);
    connect(exitAction, &QAction::triggered, this, &QApplication::quit);

    trayMenu->addAction(settingsaction);
    trayMenu->addAction(startupAction);
    trayMenu->addSeparator();
    trayMenu->addAction(mixerOnly);
    trayMenu->addAction(linkIO);
    trayMenu->addAction(exitAction);

    trayIcon->setContextMenu(trayMenu);
    trayIcon->setToolTip("Quick Sound Switcher");
    trayIcon->show();
    connect(trayIcon, &QSystemTrayIcon::activated, this, &QuickSoundSwitcher::trayIconActivated);
}

void QuickSoundSwitcher::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger) {
        togglePanel();
    }
}

void QuickSoundSwitcher::onSettingsActionActivated()
{
    if (isPanelVisible) hidePanel();
    settingsEngine->loadFromModule("Odizinne.QuickSoundSwitcher", "SettingsWindow");
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

LRESULT CALLBACK QuickSoundSwitcher::MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION) {
        if (wParam == WM_LBUTTONDOWN || wParam == WM_RBUTTONDOWN) {
            QPoint cursorPos = QCursor::pos();
            QRect trayIconRect = instance->trayIcon->geometry();
            QRect soundPanelRect;

            if (instance->panelWindow && instance->isPanelVisible) {
                soundPanelRect = instance->panelWindow->geometry();
            } else {
                soundPanelRect = QRect();
            }

            validMouseDown = !soundPanelRect.contains(cursorPos) && !trayIconRect.contains(cursorPos);
        }
        else if ((wParam == WM_LBUTTONUP || wParam == WM_RBUTTONUP) && validMouseDown) {
            QPoint cursorPos = QCursor::pos();
            QRect trayIconRect = instance->trayIcon->geometry();
            QRect soundPanelRect;

            if (instance->panelWindow && instance->isPanelVisible) {
                soundPanelRect = instance->panelWindow->geometry();
            } else {
                soundPanelRect = QRect();
            }

            validMouseDown = false;

            if (!soundPanelRect.contains(cursorPos) && !trayIconRect.contains(cursorPos)) {
                if (instance->isPanelVisible) {
                    instance->hidePanel();
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
    int originalVolume = AudioManager::getPlaybackVolume();
    int newVolume;
    if (up) {
        newVolume = originalVolume + 2;
    } else {
        newVolume = originalVolume - 2;
    }

    // Clamp the volume to valid range
    newVolume = qMax(0, qMin(100, newVolume));

    AudioManager::setPlaybackVolume(newVolume);

    if (AudioManager::getPlaybackMute()) {
        AudioManager::setPlaybackMute(false);
    }

    trayIcon->setIcon(QIcon(Utils::getIcon(1, newVolume, NULL)));

    // Update the bridge with the new volume if it exists
    if (SoundPanelBridge::instance()) {
        SoundPanelBridge::instance()->updateVolumeFromTray(newVolume);
    }
}

void QuickSoundSwitcher::toggleMuteWithKey()
{
    bool currentMuted = AudioManager::getPlaybackMute();
    bool newMuted = !currentMuted;
    int volume = AudioManager::getPlaybackVolume();

    AudioManager::setPlaybackMute(newMuted);

    int volumeIcon;
    if (volume == 0 || newMuted) {
        volumeIcon = 0;
    } else {
        volumeIcon = volume;
    }

    trayIcon->setIcon(QIcon(Utils::getIcon(1, volumeIcon, NULL)));

    // Update the bridge with the new mute state if it exists
    if (SoundPanelBridge::instance()) {
        SoundPanelBridge::instance()->updateMuteStateFromTray(newMuted);
    }
}

void QuickSoundSwitcher::togglePanel()
{
    if (!isPanelVisible) {
        showPanel();
    } else {
        hidePanel();
    }
}

void QuickSoundSwitcher::showPanel()
{
    if (!isPanelVisible) {
        // Create fresh QML engine
        createQMLEngine();

        if (panelWindow) {
            // Initialize data through the bridge
            if (SoundPanelBridge::instance()) {
                SoundPanelBridge::instance()->initializeData();
            }

            QMetaObject::invokeMethod(panelWindow, "showPanel");
            isPanelVisible = true;
            installGlobalMouseHook();
        }
    }
}

void QuickSoundSwitcher::hidePanel()
{
    if (panelWindow && isPanelVisible) {
        QMetaObject::invokeMethod(panelWindow, "hidePanel");
        // Note: We don't set isPanelVisible = false here
        // That happens in onPanelHideAnimationFinished()
    }
}

void QuickSoundSwitcher::onPanelHideAnimationFinished()
{
    isPanelVisible = false;
    uninstallGlobalMouseHook();

    // Destroy the QML engine after the animation is complete
    QTimer::singleShot(50, this, [this]() {
        destroyQMLEngine();
    });
}

void QuickSoundSwitcher::onMixerOnlyStateChanged()
{
    QAction *action = qobject_cast<QAction *>(sender());
    settings.setValue("mixerOnly", action->isChecked());

    // Notify the bridge about the mixer only state change if it exists
    if (SoundPanelBridge::instance()) {
        SoundPanelBridge::instance()->refreshMixerOnlyState();
    }

    if (settings.value("mixerOnly").toBool()) {
        QString theme = Utils::getTheme();
        if (theme == "light") {
            trayIcon->setIcon(QIcon(":/icons/system_light.png"));
        } else {
            trayIcon->setIcon(QIcon(":/icons/system_dark.png"));
        }
    } else {
        onOutputMuteChanged();
    }
}

void QuickSoundSwitcher::onLinkIOStateChanged()
{
    QAction *action = qobject_cast<QAction *>(sender());
    settings.setValue("linkIO", action->isChecked());
}

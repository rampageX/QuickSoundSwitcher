#include "quicksoundswitcher.h"
#include "soundpanelbridge.h"
#include "utils.h"
#include "audiomanager.h"
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
    , settingsEngine(new QQmlApplicationEngine(this))
    , keepAliveEngine(new QQmlApplicationEngine(this))
    , panelWindow(nullptr)
    , settingsWindow(nullptr)
    , isPanelVisible(false)
    , settings("Odizinne", "QuickSoundSwitcher")
{
    AudioManager::initialize();
    instance = this;

    // Connect to audio worker for tray icon updates
    if (AudioManager::getWorker()) {
        connect(AudioManager::getWorker(), &AudioWorker::playbackVolumeChanged,
                this, &QuickSoundSwitcher::onOutputMuteChanged);
        connect(AudioManager::getWorker(), &AudioWorker::playbackMuteChanged,
                this, &QuickSoundSwitcher::onOutputMuteChanged);
    }

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

    settingsEngine->loadFromModule("Odizinne.QuickSoundSwitcher", "SettingsWindow");
    settingsWindow = qobject_cast<QWindow*>(settingsEngine->rootObjects().first());
    keepAliveEngine->loadFromModule("Odizinne.QuickSoundSwitcher", "KeepAlive");
}

QuickSoundSwitcher::~QuickSoundSwitcher()
{
    AudioManager::cleanup();
    uninstallGlobalMouseHook();
    uninstallKeyboardHook();
    destroyQMLEngine();
    instance = nullptr;
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

            connect(panelWindow, SIGNAL(hideAnimationFinished()),
                    this, SLOT(onPanelHideAnimationFinished()));
        }
    }

    if (SoundPanelBridge::instance()) {
        connect(SoundPanelBridge::instance(), &SoundPanelBridge::shouldUpdateTray,
                this, &QuickSoundSwitcher::onOutputMuteChanged);

        // Connect to data initialization complete signal
        connect(SoundPanelBridge::instance(), &SoundPanelBridge::dataInitializationComplete,
                this, &QuickSoundSwitcher::onDataInitializationComplete);
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

    if (settings.value("panelMode", 0).toInt() == 1) {
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

    QAction *exitAction = new QAction(tr("Exit"), this);
    connect(exitAction, &QAction::triggered, this, &QApplication::quit);

    trayMenu->addAction(settingsaction);
    trayMenu->addSeparator();
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
    settingsWindow->setProperty("visible", true);
}

void QuickSoundSwitcher::onOutputMuteChanged()
{
    // Use the cached getters (now non-blocking)
    int volumeIcon;
    if (AudioManager::getPlaybackMute()) {
        volumeIcon = 0;
    } else {
        volumeIcon = AudioManager::getPlaybackVolume();
    }

    trayIcon->setIcon(QIcon(Utils::getIcon(1, volumeIcon, NULL)));
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
    // Use cached getter (non-blocking)
    int originalVolume = AudioManager::getPlaybackVolume();
    int newVolume;
    if (up) {
        newVolume = originalVolume + 2;
    } else {
        newVolume = originalVolume - 2;
    }

    newVolume = qMax(0, qMin(100, newVolume));

    // Use async for setting volume
    AudioManager::setPlaybackVolumeAsync(newVolume);

    // Use cached getter (non-blocking)
    if (AudioManager::getPlaybackMute()) {
        AudioManager::setPlaybackMuteAsync(false);
    }

    // Update tray icon immediately with new volume for responsiveness
    trayIcon->setIcon(QIcon(Utils::getIcon(1, newVolume, NULL)));

    if (SoundPanelBridge::instance()) {
        SoundPanelBridge::instance()->updateVolumeFromTray(newVolume);
    }
}

void QuickSoundSwitcher::toggleMuteWithKey()
{
    // Use cached getters (non-blocking)
    bool currentMuted = AudioManager::getPlaybackMute();
    bool newMuted = !currentMuted;
    int volume = AudioManager::getPlaybackVolume();

    // Use async for setting mute
    AudioManager::setPlaybackMuteAsync(newMuted);

    int volumeIcon;
    if (volume == 0 || newMuted) {
        volumeIcon = 0;
    } else {
        volumeIcon = volume;
    }

    // Update tray icon immediately for responsiveness
    trayIcon->setIcon(QIcon(Utils::getIcon(1, volumeIcon, NULL)));

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
        createQMLEngine();
        if (panelWindow && SoundPanelBridge::instance()) {
            // Start loading data but don't show panel yet
            SoundPanelBridge::instance()->initializeData();

            isPanelVisible = true;
            installGlobalMouseHook();

            // Panel will animate in when data is ready (handled by onDataInitializationComplete)
        }
    }
}

void QuickSoundSwitcher::onDataInitializationComplete()
{
    if (panelWindow && isPanelVisible) {
        QMetaObject::invokeMethod(panelWindow, "showPanel");
    }
}

void QuickSoundSwitcher::hidePanel()
{
    if (panelWindow && isPanelVisible) {
        QMetaObject::invokeMethod(panelWindow, "hidePanel");
    }
}

void QuickSoundSwitcher::onPanelHideAnimationFinished()
{
    isPanelVisible = false;
    uninstallGlobalMouseHook();

    QTimer::singleShot(50, this, [this]() {
        destroyQMLEngine();
    });
}

#include "quicksoundswitcher.h"
#include "soundpanelbridge.h"
#include "utils.h"
#include "audiomanager.h"
#include "mediasessionmanager.h"
#include <QMenu>
#include <QApplication>
#include <QScreen>
#include <QRect>
#include <QWindow>
#include <QQmlContext>
#include <QTimer>
#include <QFontMetrics>
#include <Windows.h>

HHOOK QuickSoundSwitcher::keyboardHook = NULL;
HHOOK QuickSoundSwitcher::mouseHook = NULL;
QuickSoundSwitcher* QuickSoundSwitcher::instance = nullptr;
static bool validMouseDown = false;

QuickSoundSwitcher::QuickSoundSwitcher(QWidget *parent)
    : QWidget(parent)
    , trayIcon(new QSystemTrayIcon(this))
    , engine(nullptr)
    , panelWindow(nullptr)
    , isPanelVisible(false)
    , settings("Odizinne", "QuickSoundSwitcher")
    , outputDeviceAction(nullptr)
    , inputDeviceAction(nullptr)
    , currentOutputDevice("")
    , currentInputDevice("")
{
    AudioManager::initialize();
    MediaSessionManager::initialize();
    instance = this;

    // Initialize QML engine once
    initializeQMLEngine();

    // Connect to audio worker for tray icon updates
    if (AudioManager::getWorker()) {
        connect(AudioManager::getWorker(), &AudioWorker::playbackVolumeChanged,
                this, &QuickSoundSwitcher::onOutputMuteChanged);
        connect(AudioManager::getWorker(), &AudioWorker::playbackMuteChanged,
                this, &QuickSoundSwitcher::onOutputMuteChanged);

        // Connect for tray menu updates
        connect(AudioManager::getWorker(), &AudioWorker::playbackVolumeChanged,
                this, &QuickSoundSwitcher::updateTrayMenuDeviceInfo);
        connect(AudioManager::getWorker(), &AudioWorker::playbackMuteChanged,
                this, &QuickSoundSwitcher::updateTrayMenuDeviceInfo);
        connect(AudioManager::getWorker(), &AudioWorker::recordingVolumeChanged,
                this, &QuickSoundSwitcher::updateTrayMenuDeviceInfo);
        connect(AudioManager::getWorker(), &AudioWorker::recordingMuteChanged,
                this, &QuickSoundSwitcher::updateTrayMenuDeviceInfo);
        connect(AudioManager::getWorker(), &AudioWorker::defaultEndpointChanged,
                this, &QuickSoundSwitcher::initializeTrayMenuDeviceInfo);
    }

    if (SoundPanelBridge::instance()) {
        connect(SoundPanelBridge::instance(), &SoundPanelBridge::languageChanged,
                this, &QuickSoundSwitcher::onLanguageChanged);
    }

    createTrayIcon();
    installKeyboardHook();

    // Initialize device info after a short delay to let everything settle
    QTimer::singleShot(1000, this, &QuickSoundSwitcher::initializeTrayMenuDeviceInfo);

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
    if (SoundPanelBridge::instance()) {
        bool chatMixEnabled = QSettings("Odizinne", "QuickSoundSwitcher").value("chatMixEnabled", false).toBool();
        if (chatMixEnabled) {
            SoundPanelBridge::instance()->restoreOriginalVolumes();

            QEventLoop loop;
            QTimer::singleShot(200, &loop, &QEventLoop::quit);
            loop.exec();
        }
    }

    AudioManager::cleanup();
    MediaSessionManager::cleanup();
    uninstallGlobalMouseHook();
    uninstallKeyboardHook();
    destroyQMLEngine();
    instance = nullptr;
}

void QuickSoundSwitcher::initializeQMLEngine()
{
    if (engine) {
        return; // Already initialized
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

    // Add device info actions (disabled)
    outputDeviceAction = new QAction(tr("Output: Loading..."), this);
    //outputDeviceAction->setEnabled(false);
    trayMenu->addAction(outputDeviceAction);

    inputDeviceAction = new QAction(tr("Input: Loading..."), this);
    //inputDeviceAction->setEnabled(false);
    trayMenu->addAction(inputDeviceAction);

    trayMenu->addSeparator();

    QAction *exitAction = new QAction(tr("Exit"), this);
    connect(exitAction, &QAction::triggered, this, &QApplication::quit);
    trayMenu->addAction(exitAction);

    trayIcon->setContextMenu(trayMenu);
    trayIcon->setToolTip("Quick Sound Switcher");
    trayIcon->show();
    connect(trayIcon, &QSystemTrayIcon::activated, this, &QuickSoundSwitcher::trayIconActivated);
}

void QuickSoundSwitcher::initializeTrayMenuDeviceInfo()
{
    if (!AudioManager::getWorker()) return;

    // Get device names
    connect(AudioManager::getWorker(), &AudioWorker::playbackDevicesReady,
            this, [this](const QList<AudioDevice>& devices) {
                for (const AudioDevice& device : devices) {
                    if (device.isDefault) {
                        currentOutputDevice = device.shortName.isEmpty() ? device.name : device.shortName;
                        updateTrayMenuDeviceInfo();
                        break;
                    }
                }
            }, Qt::SingleShotConnection);

    connect(AudioManager::getWorker(), &AudioWorker::recordingDevicesReady,
            this, [this](const QList<AudioDevice>& devices) {
                for (const AudioDevice& device : devices) {
                    if (device.isDefault) {
                        currentInputDevice = device.shortName.isEmpty() ? device.name : device.shortName;
                        updateTrayMenuDeviceInfo();
                        break;
                    }
                }
            }, Qt::SingleShotConnection);

    AudioManager::enumeratePlaybackDevicesAsync();
    AudioManager::enumerateRecordingDevicesAsync();
}

void QuickSoundSwitcher::updateTrayMenuDeviceInfo()
{
    if (!outputDeviceAction || !inputDeviceAction) return;

    // Get current volumes and mute states (from cache, non-blocking)
    int outputVolume = AudioManager::getPlaybackVolume();
    int inputVolume = AudioManager::getRecordingVolume();
    bool outputMuted = AudioManager::getPlaybackMute();
    bool inputMuted = AudioManager::getRecordingMute();

    // Update output device info
    QString outputText;
    if (!currentOutputDevice.isEmpty()) {
        outputText = QString("Output: %1").arg(elideDeviceText(currentOutputDevice, outputVolume, outputMuted));
    } else {
        outputText = QString("Output: %1%").arg(outputVolume);
    }
    outputDeviceAction->setText(outputText);

    // Update input device info
    QString inputText;
    if (!currentInputDevice.isEmpty()) {
        inputText = QString("Input: %1").arg(elideDeviceText(currentInputDevice, inputVolume, inputMuted));
    } else {
        inputText = QString("Input: %1%").arg(inputVolume);
    }
    inputDeviceAction->setText(inputText);
}

QString QuickSoundSwitcher::elideDeviceText(const QString& deviceName, int volume, bool muted)
{
    QFontMetrics metrics(QApplication::font());

    // Calculate available width (menu width minus some padding)
    int maxWidth = 250;

    QString volumeText = QString(" %1%").arg(volume);
    QString prefix = "Output: "; // Account for the prefix
    int availableWidth = maxWidth - metrics.horizontalAdvance(prefix + volumeText);

    QString elidedName = metrics.elidedText(deviceName, Qt::ElideRight, availableWidth);
    return QString("%1%2").arg(elidedName, volumeText);
}

void QuickSoundSwitcher::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger) {
        togglePanel();
    }
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
    if (!isPanelVisible && panelWindow) {
        if (SoundPanelBridge::instance()) {
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
    // No need to destroy engine anymore - just keep it hidden
}

void QuickSoundSwitcher::onLanguageChanged()
{
    if (engine) {
        engine->retranslate();
    }

    updateTrayMenu();
}

void QuickSoundSwitcher::updateTrayMenu()
{
    if (!trayIcon || !trayIcon->contextMenu()) {
        return;
    }

    // Get the current menu
    QMenu *trayMenu = trayIcon->contextMenu();

    // Update the Exit action text
    QList<QAction*> actions = trayMenu->actions();
    for (QAction* action : actions) {
        if (action->text().contains("Exit") || action->text().contains("Quitter") ||
            action->text().contains("Esci") || action->text().contains("Beenden")) {
            action->setText(tr("Exit"));
            break;
        }
    }

    // Update device info actions text
    if (outputDeviceAction) {
        QString currentText = outputDeviceAction->text();
        // Extract the device info part and rebuild with translated prefix
        int colonIndex = currentText.indexOf(':');
        if (colonIndex != -1) {
            QString deviceInfo = currentText.mid(colonIndex);
            outputDeviceAction->setText(tr("Output") + deviceInfo);
        }
    }

    if (inputDeviceAction) {
        QString currentText = inputDeviceAction->text();
        int colonIndex = currentText.indexOf(':');
        if (colonIndex != -1) {
            QString deviceInfo = currentText.mid(colonIndex);
            inputDeviceAction->setText(tr("Input") + deviceInfo);
        }
    }
}

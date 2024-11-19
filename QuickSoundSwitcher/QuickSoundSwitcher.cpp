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
#include <QPropertyAnimation>

QuickSoundSwitcher::QuickSoundSwitcher(QWidget *parent)
    : QMainWindow(parent)
    , trayIcon(new QSystemTrayIcon(this))
    , panel(nullptr)
    , hiding(false)
    , overlayWidget(nullptr)
    , overlaySettings(nullptr)
    , settings("Odizinne", "QuickSoundSwitcher")
{
    createTrayIcon();
    registerGlobalHotkey();
    loadSettings();
    toggleMutedOverlay(AudioManager::getRecordingMute());
}

QuickSoundSwitcher::~QuickSoundSwitcher()
{
    unregisterGlobalHotkey();
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

    panel = new Panel;

    connect(panel, &Panel::volumeChanged, this, &QuickSoundSwitcher::onVolumeChanged);
    connect(panel, &Panel::outputMuteChanged, this, &QuickSoundSwitcher::onOutputMuteChanged);
    connect(panel, &Panel::inputMuteChanged, this, &QuickSoundSwitcher::onInputMuteChanged);
    connect(panel, &Panel::lostFocus, this, &QuickSoundSwitcher::hidePanel);

    QRect trayIconGeometry = trayIcon->geometry();
    QPoint trayIconPos = trayIconGeometry.topLeft();
    int trayIconCenterX = trayIconPos.x() + trayIconGeometry.width() / 2;

    int panelX = trayIconCenterX - panel->width() / 2; // Center horizontally
    QRect screenGeometry = QApplication::primaryScreen()->geometry();
    int startY = screenGeometry.bottom();  // Start from the bottom of the screen
    int targetY = trayIconGeometry.top() - panel->height() - 12; // Final position

    panel->move(panelX, startY); // Start at the bottom
    panel->show();              // Ensure the panel is visible

    // Animation parameters
    const int durationMs = 300; // Total duration in milliseconds
    const int refreshRate = 8; // Timer interval (~60 FPS)
    const double totalSteps = durationMs / refreshRate;

    int currentStep = 0;
    QTimer *animationTimer = new QTimer(this);

    animationTimer->start(refreshRate); // ~60 updates per second

    connect(animationTimer, &QTimer::timeout, this, [=]() mutable {
        if (currentStep >= totalSteps) {
            animationTimer->stop();
            animationTimer->deleteLater();
            panel->move(panelX, targetY); // Ensure final position is set
            return;
        }

        double t = static_cast<double>(currentStep) / totalSteps; // Normalized time (0 to 1)
        // Easing function: Smooth deceleration
        double easedT = 1 - pow(1 - t, 3);

        // Interpolated Y position
        int currentY = startY + easedT * (targetY - startY);
        panel->move(panelX, currentY);

        ++currentStep;
    });
}

void QuickSoundSwitcher::hidePanel()
{
    if (!panel) return;
    hiding = true;

    QRect trayIconGeometry = trayIcon->geometry();
    QPoint trayIconPos = trayIconGeometry.topLeft();
    int trayIconCenterX = trayIconPos.x() + trayIconGeometry.width() / 2;

    int panelX = trayIconCenterX - panel->width() / 2; // Center horizontally
    QRect screenGeometry = QApplication::primaryScreen()->geometry();
    int startY = panel->y();
    int targetY = screenGeometry.bottom(); // Move to the bottom of the screen

    // Animation parameters
    const int durationMs = 300; // Total duration in milliseconds
    const int refreshRate = 8; // Timer interval (~60 FPS)
    const double totalSteps = durationMs / refreshRate;

    int currentStep = 0;
    QTimer *animationTimer = new QTimer(this);

    animationTimer->start(refreshRate); // ~60 updates per second

    connect(animationTimer, &QTimer::timeout, this, [=]() mutable {
        if (currentStep >= totalSteps) {
            animationTimer->stop();
            animationTimer->deleteLater();
            delete panel;
            panel = nullptr;
            hiding = false;
            return;
        }

        double t = static_cast<double>(currentStep) / totalSteps; // Normalized time (0 to 1)
        // Easing function: Smooth deceleration
        double easedT = 1 - pow(1 - t, 3);

        // Interpolated Y position
        int currentY = startY + easedT * (targetY - startY);
        panel->move(panelX, currentY);

        ++currentStep;
    });
}

void QuickSoundSwitcher::onPanelClosed()
{
    panel = nullptr;
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

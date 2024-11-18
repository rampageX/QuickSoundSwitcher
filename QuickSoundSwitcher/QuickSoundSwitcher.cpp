#include "QuickSoundSwitcher.h"
#include "Utils.h"
#include "AudioManager.h"
#include "ShortcutManager.h"
#include <QMenu>
#include <QApplication>
#include <QScreen>
#include <QRect>
#include <QTimer>

QuickSoundSwitcher::QuickSoundSwitcher(QWidget *parent)
    : QMainWindow(parent)
    , trayIcon(new QSystemTrayIcon(this))
    , panel(nullptr)
    , hiding(false)
{
    createTrayIcon();
}

QuickSoundSwitcher::~QuickSoundSwitcher() {
}

void QuickSoundSwitcher::createTrayIcon()
{
    onOutputMuteChanged();

    QMenu *trayMenu = new QMenu(this);

    connect(trayMenu, &QMenu::aboutToShow, this, [trayMenu, this]() {
        trayMenu->clear();

        QList<AudioDevice> playbackDevices;
        AudioManager::enumeratePlaybackDevices(playbackDevices);
        createDeviceSubMenu(trayMenu, playbackDevices, tr("Output device"));

        QList<AudioDevice> recordingDevices;
        AudioManager::enumerateRecordingDevices(recordingDevices);
        createDeviceSubMenu(trayMenu, recordingDevices, tr("Input device"));

        trayMenu->addSeparator();

        QAction *startupAction = new QAction(tr("Run at startup"), this);
        startupAction->setCheckable(true);
        startupAction->setChecked(ShortcutManager::isShortcutPresent());
        connect(startupAction, &QAction::triggered, this, &QuickSoundSwitcher::onRunAtStartupStateChanged);
        trayMenu->addAction(startupAction);

        QAction *exitAction = new QAction(tr("Exit"), this);
        connect(exitAction, &QAction::triggered, this, &QApplication::quit);
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
    connect(panel, &Panel::lostFocus, this, &QuickSoundSwitcher::hidePanel);

    QRect trayIconGeometry = trayIcon->geometry();
    QPoint trayIconPos = trayIconGeometry.topLeft();
    int trayIconCenterX = trayIconPos.x() + trayIconGeometry.width() / 2;

    int panelX = trayIconCenterX - panel->width() / 2;
    int panelY = trayIconPos.y() - panel->height() - 12;

    panel->move(panelX, panelY);
    panel->fadeIn();
}

void QuickSoundSwitcher::hidePanel()
{
    if (!panel) return;
    hiding = true;
    disconnect(panel, &Panel::lostFocus, this, &QuickSoundSwitcher::hidePanel);

    connect(panel, &Panel::fadeOutFinished, this, [this]() {
        delete panel;
        panel = nullptr;
        hiding = false;
    });

    panel->fadeOut();
}

void QuickSoundSwitcher::movePanelDown()
{
    if (currentY < targetY) {
        currentY += 5;
        panel->move(panel->x(), currentY);
    } else {
        delete panel;
        panel = nullptr;

        QTimer* timer = qobject_cast<QTimer*>(sender());
        if (timer) {
            timer->stop();
            timer->deleteLater();
        }
    }
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

void QuickSoundSwitcher::onRunAtStartupStateChanged()
{
    QAction *action = qobject_cast<QAction *>(sender());
    ShortcutManager::manageShortcut(action->isChecked());
}

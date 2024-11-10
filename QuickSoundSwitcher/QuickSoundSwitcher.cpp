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
    trayIcon->setIcon(Utils::getIcon(1, AudioManager::getPlaybackVolume(), NULL));

    QMenu *trayMenu = new QMenu(this);

    QAction *startupAction = new QAction("Run at startup", this);
    startupAction->setCheckable(true);
    startupAction->setChecked(ShortcutManager::isShortcutPresent());
    connect(startupAction, &QAction::triggered, this, &QuickSoundSwitcher::onRunAtStartupStateChanged);
    trayMenu->addAction(startupAction);

    QAction *exitAction = new QAction("Exit", this);
    connect(exitAction, &QAction::triggered, this, &QApplication::quit);
    trayMenu->addAction(exitAction);

    trayIcon->setContextMenu(trayMenu);
    trayIcon->setToolTip("Quick Sound Switcher");
    trayIcon->show();
    connect(trayIcon, &QSystemTrayIcon::activated, this, &QuickSoundSwitcher::trayIconActivated);
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
    connect(panel, &Panel::lostFocus, this, &QuickSoundSwitcher::hidePanel);

    QRect trayIconGeometry = trayIcon->geometry();
    QPoint trayIconPos = trayIconGeometry.topLeft();
    int trayIconCenterX = trayIconPos.x() + trayIconGeometry.width() / 2;

    int panelX = trayIconCenterX - panel->width() / 2;
    int panelY = trayIconPos.y() - panel->height() - 13;

    panel->move(panelX, panelY);

    connect(panel, &Panel::volumeChanged, this, &QuickSoundSwitcher::onVolumeChanged);
    connect(panel, &Panel::lostFocus, this, &QuickSoundSwitcher::hidePanel);

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

void QuickSoundSwitcher::onRunAtStartupStateChanged()
{
    QAction *action = qobject_cast<QAction *>(sender());
    ShortcutManager::manageShortcut(action->isChecked());
}

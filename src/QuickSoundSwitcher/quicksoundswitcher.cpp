#include "quicksoundswitcher.h"
#include "utils.h"
#include "audiomanager.h"
#include "shortcutmanager.h"
#include <QMenu>
#include <QApplication>
#include <QScreen>
#include <QRect>
#include <QTimer>

using namespace Utils;
using namespace AudioManager;
using namespace ShortcutManager;

QuickSoundSwitcher::QuickSoundSwitcher(QWidget *parent)
    : QMainWindow(parent)
    , trayIcon(new QSystemTrayIcon(this))
    , panel(nullptr)
{
    createTrayIcon();
}

QuickSoundSwitcher::~QuickSoundSwitcher() {

}

void QuickSoundSwitcher::createTrayIcon()
{
    trayIcon->setIcon(getIcon(1, getPlaybackVolume(), NULL));

    QMenu *trayMenu = new QMenu(this);

    QAction *startupAction = new QAction("Run at startup", this);
    startupAction->setCheckable(true);
    startupAction->setChecked(isShortcutPresent());
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
    if (panel) {
        panel->showNormal();
        panel->raise();
        panel->activateWindow();
        return;
    }

    panel = new Panel;

    connect(panel, &Panel::volumeChanged, this, &QuickSoundSwitcher::onVolumeChanged);
    connect(panel, &Panel::lostFocus, this, &QuickSoundSwitcher::hidePanel);

    // Get the geometry of the tray icon
    QRect iconGeometry = trayIcon->geometry();

    // Calculate the center position of the tray icon
    int iconCenterX = iconGeometry.x() + iconGeometry.width() / 2;
    int iconCenterY = iconGeometry.y() + iconGeometry.height() / 2;

    // Calculate the target position for the panel (centering it above the tray icon)
    int panelX = iconCenterX - (panel->width() / 2); // Center the panel horizontally
    int panelY = iconCenterY - panel->height() - 35; // Set Y position 35 pixels above the tray icon

    // Set initial position for the panel (offscreen or hidden)
    panel->move(panelX, iconCenterY + 50); // Start position below the tray icon

    // Show the panel to ensure it's created
    panel->show();

    // Create a timer for moving the panel
    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &QuickSoundSwitcher::movePanelUp);

    targetY = panelY; // Set target Y position
    currentY = iconCenterY + 50; // Initial Y position
    timer->start(4); // Reduced interval for faster movement
}

void QuickSoundSwitcher::movePanelUp()
{
    if (currentY > targetY) {
        currentY -= 5;
        panel->move(panel->x(), currentY);
    } else {
        panel->move(panel->x(), targetY);
        panel->raise();
        panel->activateWindow();
        QTimer* timer = qobject_cast<QTimer*>(sender());
        if (timer) {
            timer->stop();
            timer->deleteLater();
        }
    }
}

void QuickSoundSwitcher::hidePanel()
{
    if (!panel) return;
    disconnect(panel, &Panel::lostFocus, this, &QuickSoundSwitcher::hidePanel);

    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &QuickSoundSwitcher::movePanelDown);

    currentY = panel->y();
    targetY = trayIcon->geometry().y() + trayIcon->geometry().height() + 50;
    timer->start(4);
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
    trayIcon->setIcon(getIcon(1, getPlaybackVolume(), NULL));
}

void QuickSoundSwitcher::onRunAtStartupStateChanged()
{
    QAction *action = qobject_cast<QAction *>(sender());
    manageShortcut(action->isChecked());
}

#include "quicksoundswitcher.h"
#include "utils.h"
#include "audiomanager.h"
#include "shortcutmanager.h"
#include <QMenu>
#include <QApplication>
#include <QScreen>
#include <QRect>

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
    panel->setAttribute(Qt::WA_DeleteOnClose);
    connect(panel, &Panel::closed, this, &QuickSoundSwitcher::onPanelClosed);
    connect(panel, &Panel::volumeChanged, this, &QuickSoundSwitcher::onVolumeChanged);

    // Get the geometry of the tray icon
    QRect iconGeometry = trayIcon->geometry();

    // Calculate the center position of the tray icon
    int iconCenterX = iconGeometry.x() + iconGeometry.width() / 2;
    int iconCenterY = iconGeometry.y() + iconGeometry.height() / 2;

    // Calculate the position for the panel (centering it above the tray icon)
    int panelX = iconCenterX - (panel->width() / 2); // Center the panel horizontally
    int panelY = iconCenterY - panel->height() - 35; // Set Y position 10 pixels above the tray icon's center

    // Move the panel to the calculated position
    panel->move(panelX, panelY);
    panel->show();
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

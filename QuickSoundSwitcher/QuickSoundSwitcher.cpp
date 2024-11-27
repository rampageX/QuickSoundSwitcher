#include "QuickSoundSwitcher.h"
#include "Utils.h"
#include "AudioManager.h"
#include "ShortcutManager.h"
#include <QMenu>
#include <QApplication>
#include <QScreen>
#include <QRect>
#include <QTimer>
#include <QThread>
#include <Windows.h>

HHOOK QuickSoundSwitcher::keyboardHook = NULL;
HHOOK QuickSoundSwitcher::mouseHook = NULL;
QuickSoundSwitcher* QuickSoundSwitcher::instance = nullptr;

QuickSoundSwitcher::QuickSoundSwitcher(QWidget *parent)
    : QMainWindow(parent)
    , trayIcon(new QSystemTrayIcon(this))
    , settings("Odizinne", "QuickSoundSwitcher")
    , panel(nullptr)
    , mediaFlyout(nullptr)
    , soundOverlay(nullptr)
    , settingsPage(nullptr)
    , workerThread(nullptr)
    , worker(nullptr)
    , mediaSessionTimer(nullptr)
{    
    instance = this;
    createTrayIcon();
    updateApplicationColorScheme();
    loadSettings();
    installGlobalMouseHook();
    installKeyboardHook();
}

QuickSoundSwitcher::~QuickSoundSwitcher()
{
    uninstallGlobalMouseHook();
    uninstallKeyboardHook();
    delete soundOverlay;
    delete panel;
    delete settingsPage;
    stopMonitoringMediaSession();
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

    QAction *settingsAction = new QAction(tr("Settings"), this);
    connect(settingsAction, &QAction::triggered, this, &QuickSoundSwitcher::showSettings);

    QAction *exitAction = new QAction(tr("Exit"), this);
    connect(exitAction, &QAction::triggered, this, &QApplication::quit);

    trayMenu->addAction(startupAction);
    trayMenu->addAction(settingsAction);
    trayMenu->addSeparator();
    trayMenu->addAction(exitAction);

    trayIcon->setContextMenu(trayMenu);
    trayIcon->setToolTip("Quick Sound Switcher");
    trayIcon->show();
    connect(trayIcon, &QSystemTrayIcon::activated, this, &QuickSoundSwitcher::trayIconActivated);
}

bool QuickSoundSwitcher::event(QEvent *event)
{
    if (event->type() == QEvent::ApplicationPaletteChange) {
        updateApplicationColorScheme();
        onOutputMuteChanged();
        return true;
    }
    return QMainWindow::event(event);
}

void QuickSoundSwitcher::updateApplicationColorScheme()
{
    QString mode = Utils::getTheme();
    QString color;
    QPalette palette = QApplication::palette();

    if (mode == "light") {
        color = Utils::getAccentColor("light1");
    } else {
        color = Utils::getAccentColor("dark1");
    }

    QColor highlightColor(color);

    palette.setColor(QPalette::Highlight, highlightColor);
    qApp->setPalette(palette);
}

void QuickSoundSwitcher::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger) {
        showPanel();
    }
}

void QuickSoundSwitcher::showPanel()
{
    if (panel && mediaFlyout) {
        hidePanel();
        return;
    }

    panel = new Panel(this);
    panel->mergeApps = mergeSimilarApps;
    panel->populateApplications();
    connect(panel, &Panel::volumeChanged, this, &QuickSoundSwitcher::onVolumeChanged);
    connect(panel, &Panel::outputMuteChanged, this, &QuickSoundSwitcher::onOutputMuteChanged);
    connect(panel, &Panel::lostFocus, this, &QuickSoundSwitcher::hidePanel);
    connect(panel, &Panel::panelClosed, this, &QuickSoundSwitcher::onPanelClosed);

    panel->animateIn(trayIcon->geometry());

    getMediaSession();
}

void QuickSoundSwitcher::hidePanel()
{
    stopMonitoringMediaSession();
    panel->animateOut(trayIcon->geometry());
    mediaFlyout->animateOut(trayIcon->geometry());
}

void QuickSoundSwitcher::onPanelClosed()
{
    delete panel;
    panel = nullptr;
    delete mediaFlyout;
    mediaFlyout = nullptr;
}

void QuickSoundSwitcher::onSoundOverlayClosed()
{
    delete soundOverlay;
    soundOverlay = nullptr;
}

void QuickSoundSwitcher::onVolumeChanged()
{
    int volumeIcon;
    if (AudioManager::getPlaybackMute()) {
        volumeIcon = 0;
    } else {
        volumeIcon = AudioManager::getPlaybackVolume();
    }

    trayIcon->setIcon(Utils::getIcon(1, volumeIcon, NULL));
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
    ShortcutManager::manageShortcut(action->isChecked(), "QuickSoundSwitcher.lnk");
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

void QuickSoundSwitcher::toggleMicMute()
{
    AudioManager::setRecordingMute(!AudioManager::getRecordingMute());

    if (panel) {
        emit muteStateChanged();
    }
}

void QuickSoundSwitcher::showSettings()
{
    if (settingsPage) {
        settingsPage->showNormal();
        settingsPage->raise();
        settingsPage->activateWindow();
        return;
    }

    settingsPage = new SettingsPage;
    settingsPage->setAttribute(Qt::WA_DeleteOnClose);
    connect(settingsPage, &SettingsPage::closed, this, &QuickSoundSwitcher::onSettingsClosed);
    connect(settingsPage, &SettingsPage::settingsChanged, this, &QuickSoundSwitcher::onSettingsChanged);
    settingsPage->show();
}

void QuickSoundSwitcher::loadSettings()
{
    volumeIncrement = settings.value("volumeIncrement", 2).toInt();
    mergeSimilarApps = settings.value("mergeSimilarApps", true).toBool();
}

void QuickSoundSwitcher::onSettingsChanged()
{
    loadSettings();
}

void QuickSoundSwitcher::onSettingsClosed()
{
    disconnect(settingsPage, &SettingsPage::closed, this, &QuickSoundSwitcher::onSettingsClosed);
    disconnect(settingsPage, &SettingsPage::settingsChanged, this, &QuickSoundSwitcher::onSettingsChanged);
    settingsPage = nullptr;
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

        if (wParam == WM_LBUTTONDOWN || wParam == WM_RBUTTONDOWN) {
            QPoint cursorPos = QCursor::pos();
            QRect trayIconRect = instance->trayIcon->geometry();
            QRect panelRect = instance->panel ? instance->panel->geometry() : QRect();
            QRect mediaFlyoutRect = instance->mediaFlyout ? instance->mediaFlyout->geometry() : QRect();

            if (!trayIconRect.contains(cursorPos) &&
                !panelRect.contains(cursorPos) &&
                !mediaFlyoutRect.contains(cursorPos)) {
                if (instance->panel) {
                    emit instance->panel->lostFocus();
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
    int newVolume;
    if (up) {
        newVolume = volume + volumeIncrement;
        if (newVolume > 100) {
            newVolume = 100;
        }
    } else {
        newVolume = volume - volumeIncrement;
        if (newVolume < 0) {
            newVolume = 0;
        }
    }

    AudioManager::setPlaybackVolume(newVolume);

    if (AudioManager::getPlaybackMute()) {
        AudioManager::setPlaybackMute(false);
        emit outputMuteStateChanged(false);
    }
    trayIcon->setIcon(Utils::getIcon(1, newVolume, NULL));
    emit volumeChangedWithTray();

    if (!soundOverlay) {
        soundOverlay = new SoundOverlay(this);
        connect(soundOverlay, &SoundOverlay::overlayClosed, this, &QuickSoundSwitcher::onSoundOverlayClosed);
    }

    soundOverlay->animateIn();
    soundOverlay->updateVolumeIconAndLabel(Utils::getIcon(1, newVolume, NULL), newVolume);
}

void QuickSoundSwitcher::toggleMuteWithKey()
{
    bool newPlaybackMute = !AudioManager::getPlaybackMute();
    AudioManager::setPlaybackMute(newPlaybackMute);

    int volumeIcon;
    if (AudioManager::getPlaybackMute()) {
        volumeIcon = 0;
    } else {
        volumeIcon = AudioManager::getPlaybackVolume();
    }

    trayIcon->setIcon(Utils::getIcon(1, volumeIcon, NULL));

    emit outputMuteStateChanged(newPlaybackMute);

    if (!soundOverlay) {
        soundOverlay = new SoundOverlay(this);
    }

    soundOverlay->updateMuteIcon(Utils::getIcon(1, volumeIcon, NULL));
    soundOverlay->updateVolumeIconAndLabel(Utils::getIcon(1, volumeIcon, NULL), AudioManager::getPlaybackVolume());
    soundOverlay->animateIn();
}

void QuickSoundSwitcher::stopMonitoringMediaSession()
{
    if (mediaSessionTimer) {
        mediaSessionTimer->stop();
        delete mediaSessionTimer;
        mediaSessionTimer = nullptr;
    }

    if (worker) {
        workerThread->quit();
        workerThread->wait();
        delete worker;
        worker = nullptr;
    }

    if (workerThread) {
        delete workerThread;
        workerThread = nullptr;
    }
}

void QuickSoundSwitcher::getMediaSession()
{
    if (!workerThread) workerThread = new QThread(this);
    if (!worker) {
        worker = new MediaSessionWorker();
        worker->moveToThread(workerThread);

        connect(workerThread, &QThread::started, worker, &MediaSessionWorker::process);
        connect(worker, &MediaSessionWorker::sessionReady, this, &QuickSoundSwitcher::onSessionReady);
        connect(worker, &MediaSessionWorker::sessionError, this, &QuickSoundSwitcher::onSessionError);

        workerThread->start();

    } else {
        worker->process();
    }
}

void QuickSoundSwitcher::onSessionReady(const MediaSession& session)
{
    mediaFlyout = new MediaFlyout(this);
    connect(mediaFlyout, &MediaFlyout::requestNext, this, &QuickSoundSwitcher::onRequestNext);
    connect(mediaFlyout, &MediaFlyout::requestPrev, this, &QuickSoundSwitcher::onRequestPrev);
    connect(mediaFlyout, &MediaFlyout::requestPause, this, &QuickSoundSwitcher::onRequestPause);

    mediaFlyout->animateIn();
    mediaFlyout->updateTitle(session.title);
    mediaFlyout->updateArtist(session.artist);
    mediaFlyout->updateIcon(session.icon);
    mediaFlyout->updatePauseButton(session.playbackState);
    mediaFlyout->updateControls(session.canGoPrevious, session.canGoNext);
    mediaFlyout->updateProgress(session.currentTime, session.duration);
    if (session.playbackState == "Playing") {
        currentlyPlaying = true;
    } else {
        currentlyPlaying = false;
    }

    worker->initializeSessionMonitoring();

    connect(worker, &MediaSessionWorker::sessionMediaPropertiesChanged, this, &QuickSoundSwitcher::updateFlyoutTitleAndArtist);
    connect(worker, &MediaSessionWorker::sessionIconChanged, this, &QuickSoundSwitcher::updateFlyoutIcon);
    connect(worker, &MediaSessionWorker::sessionPlaybackStateChanged, this, &QuickSoundSwitcher::updateFlyoutPlayPause);
    connect(worker, &MediaSessionWorker::sessionTimeInfoUpdated, this, &QuickSoundSwitcher::updateFlyoutProgress);
}

void QuickSoundSwitcher::onSessionError()
{
    mediaFlyout = new MediaFlyout(this);
    connect(mediaFlyout, &MediaFlyout::requestNext, this, &QuickSoundSwitcher::onRequestNext);
    connect(mediaFlyout, &MediaFlyout::requestPrev, this, &QuickSoundSwitcher::onRequestPrev);
    connect(mediaFlyout, &MediaFlyout::requestPause, this, &QuickSoundSwitcher::onRequestPause);

    mediaFlyout->animateIn();
}

void QuickSoundSwitcher::updateFlyoutTitleAndArtist(const QString& title, const QString& artist)
{
    if (mediaFlyout){
        mediaFlyout->updateTitle(title);
        mediaFlyout->updateArtist(artist);
    }
}

void QuickSoundSwitcher::updateFlyoutIcon(QIcon icon)
{
    if (mediaFlyout){
        mediaFlyout->updateIcon(icon);
    }
}

void QuickSoundSwitcher::updateFlyoutPlayPause(const QString &state)
{
    if (state == "Playing") {
        currentlyPlaying = true;
    } else {
        currentlyPlaying = false;
    }

    mediaFlyout->updatePauseButton(state);
}

void QuickSoundSwitcher::updateFlyoutProgress(int currentTime, int totalTime)
{
    mediaFlyout->updateProgress(currentTime, totalTime);
}

void QuickSoundSwitcher::onRequestNext()
{
    worker->next();
}

void QuickSoundSwitcher::onRequestPrev()
{
    worker->previous();
}

void QuickSoundSwitcher::onRequestPause()
{
    if (!currentlyPlaying) {
        worker->play();
    } else {
        worker->pause();
    }
}

#include "SoundPanel.h"
#include "Utils.h"
#include <QQmlContext>
#include <QQuickItem>
#include <QIcon>
#include <QPixmap>
#include <QBuffer>
#include <QApplication>
#include <QTimer>
#include "QuickSoundSwitcher.h"
#include <QPropertyAnimation>

SoundPanel::SoundPanel(QObject* parent)
    : QObject(parent)
    , soundPanelWindow(nullptr)
    , engine(new QQmlApplicationEngine(this))
    , isWindows10(Utils::isWindows10())
    , isAnimating(false)
    , hWnd(nullptr)
    , settings("Odizinne", "QuickSoundSwitcher")
    , mixerOnly(settings.value("mixerOnly").toBool())
    , systemSoundsMuted(false)
{
    configureQML();
    populateApplicationModel();

    soundPanelWindow = qobject_cast<QWindow*>(engine->rootObjects().first());
    hWnd = reinterpret_cast<HWND>(soundPanelWindow->winId());

    animateIn();
    setupUI(); //setup components while animating window since components are hidden until animation completes

    connect(static_cast<QuickSoundSwitcher*>(parent), &QuickSoundSwitcher::outputMuteStateChanged,
            this, &SoundPanel::onOutputMuteStateChanged);
    connect(static_cast<QuickSoundSwitcher*>(parent), &QuickSoundSwitcher::volumeChangedWithTray,
            this, &SoundPanel::onVolumeChangedWithTray);
}

SoundPanel::~SoundPanel()
{
    delete soundPanelWindow;
    delete engine;
}

void SoundPanel::animateOpacity()
{
    constexpr int fps = 60;
    constexpr int duration = 200;
    constexpr int interval = 1000 / fps;
    constexpr int steps = duration / interval;
    double currentOpacity = 0.0;
    double stepSize = 1.0 / steps;

    QTimer *timer = new QTimer();
    QObject::connect(timer, &QTimer::timeout, [=]() mutable {
        currentOpacity += stepSize;
        if (currentOpacity >= 1.0) {
            currentOpacity = 1.0;
            engine->rootContext()->setContextProperty("componentsOpacity", currentOpacity);
            timer->stop();
            timer->deleteLater();
        }
        engine->rootContext()->setContextProperty("componentsOpacity", currentOpacity);
    });

    timer->start(interval);
}

void SoundPanel::configureQML() {
    engine->rootContext()->setContextProperty("soundPanel", this);
    engine->rootContext()->setContextProperty("mixerOnly", mixerOnly);
    engine->rootContext()->setContextProperty("componentsOpacity", 0);

    engine->load(QUrl("qrc:/qml/SoundPanel10.qml"));
}

void SoundPanel::setupUI() {
    setSystemSoundsIcon();

    if (mixerOnly) return;

    populateComboBoxes();
    setPlaybackVolume(AudioManager::getPlaybackVolume());
    setRecordingVolume(AudioManager::getRecordingVolume());

    int volume = AudioManager::getPlaybackVolume();
    bool isOutputMuted = AudioManager::getPlaybackMute();

    if (isOutputMuted || volume == 0) {
        setOutputButtonImage(0);
    } else {
        setOutputButtonImage(volume);
    }

    bool isInputMuted = AudioManager::getRecordingMute();
    setInputButtonImage(isInputMuted);
}

void SoundPanel::onVolumeChangedWithTray(int volume) {
    setPlaybackVolume(volume);
    setOutputButtonImage(volume);
}

void SoundPanel::onOutputMuteStateChanged(int volumeIcon) {
    setOutputButtonImage(volumeIcon);
}

void SoundPanel::animateIn()
{
    if (isAnimating) return;

    isAnimating = true;

    QRect availableGeometry = QApplication::primaryScreen()->availableGeometry();

    int engineHeight = engine->rootObjects().first()->property("height").toInt();
    soundPanelWindow->resize(330, engineHeight);
    int margin = isWindows10 ? margin = -1 : margin = 12;
    int panelX = availableGeometry.right() - soundPanelWindow->width() + 1 - margin;

    int scalingFactor = isWindows10 ? 70 : 0;
    int startY = availableGeometry.bottom() - (soundPanelWindow->height() * scalingFactor / 100);

    int targetY = availableGeometry.bottom() - soundPanelWindow->height() - margin;
    soundPanelWindow->setPosition(panelX, startY);

    QPropertyAnimation *animation = new QPropertyAnimation(soundPanelWindow, "y", this);
    animation->setDuration(isWindows10 ? 300 : 200);
    animation->setStartValue(startY);
    animation->setEndValue(targetY);
    animation->setEasingCurve(QEasingCurve::OutQuad);

    connect(animation, &QPropertyAnimation::finished, this, [this, animation]() {
        animateOpacity();
        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        isAnimating = false;
        animation->deleteLater();
    });

    soundPanelWindow->show();
    animation->start();
}

void SoundPanel::animateOut()
{
    if (isAnimating) return;

    isAnimating = true;

    QRect availableGeometry = QApplication::primaryScreen()->availableGeometry();

    int scalingFactor = isWindows10 ? 70 : 0;
    int startY = soundPanelWindow->y();
    int targetY = availableGeometry.bottom() - (soundPanelWindow->height() * scalingFactor / 100);

    QPropertyAnimation *animation = new QPropertyAnimation(soundPanelWindow, "y", this);
    animation->setDuration(isWindows10 ? 300 : 200);
    animation->setStartValue(startY);
    animation->setEndValue(targetY);
    animation->setEasingCurve(QEasingCurve::InQuad);

    connect(animation, &QPropertyAnimation::finished, this, [this, animation]() {
        soundPanelWindow->hide();
        animation->deleteLater();
        this->deleteLater();
    });

    SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    animation->start();
}

int SoundPanel::playbackVolume() const {
    return m_playbackVolume;
}

void SoundPanel::setPlaybackVolume(int volume) {
    if (m_playbackVolume != volume) {
        m_playbackVolume = volume;
        emit playbackVolumeChanged();
    }
}

int SoundPanel::recordingVolume() const {
    return m_recordingVolume;
}

void SoundPanel::setRecordingVolume(int volume) {
    if (m_recordingVolume != volume) {
        m_recordingVolume = volume;
        emit recordingVolumeChanged();
    }
}

void SoundPanel::onPlaybackVolumeChanged(int volume) {
    AudioManager::setPlaybackVolume(volume);
    if (AudioManager::getPlaybackMute()) {
        AudioManager::setPlaybackMute(false);
    }
    setOutputButtonImage(volume);
    emit shouldUpdateTray();
}

void SoundPanel::onRecordingVolumeChanged(int volume) {
    AudioManager::setRecordingVolume(volume);
}

void SoundPanel::populateComboBoxes() {
    playbackDevices.clear();
    recordingDevices.clear();

    AudioManager::enumeratePlaybackDevices(playbackDevices);
    AudioManager::enumerateRecordingDevices(recordingDevices);

    QMetaObject::invokeMethod(engine->rootObjects().first(), "clearPlaybackDevices");

    int defaultPlaybackIndex = -1;
    for (int i = 0; i < playbackDevices.size(); ++i) {
        const AudioDevice& device = playbackDevices[i];
        if (device.shortName == "") {
            QMetaObject::invokeMethod(engine->rootObjects().first(), "addPlaybackDevice",
                                      Q_ARG(QVariant, QVariant::fromValue(device.name)));
        } else {
            QMetaObject::invokeMethod(engine->rootObjects().first(), "addPlaybackDevice",
                                      Q_ARG(QVariant, QVariant::fromValue(device.shortName)));
        }

        if (device.isDefault) {
            defaultPlaybackIndex = i;
        }
    }

    if (defaultPlaybackIndex != -1) {
        QMetaObject::invokeMethod(engine->rootObjects().first(), "setPlaybackDeviceCurrentIndex",
                                  Q_ARG(QVariant, defaultPlaybackIndex));
    }

    QMetaObject::invokeMethod(engine->rootObjects().first(), "clearRecordingDevices");

    int defaultRecordingIndex = -1;
    for (int i = 0; i < recordingDevices.size(); ++i) {
        const AudioDevice& device = recordingDevices[i];
        QMetaObject::invokeMethod(engine->rootObjects().first(), "addRecordingDevice",
                                  Q_ARG(QVariant, QVariant::fromValue(device.shortName)));

        if (device.isDefault) {
            defaultRecordingIndex = i;
        }
    }

    if (defaultRecordingIndex != -1) {
        QMetaObject::invokeMethod(engine->rootObjects().first(), "setRecordingDeviceCurrentIndex",
                                  Q_ARG(QVariant, defaultRecordingIndex));
    }
}

void SoundPanel::onPlaybackDeviceChanged(const QString &deviceName) {
    for (const AudioDevice& device : playbackDevices) {
        if (device.shortName == deviceName) {
            AudioManager::setDefaultEndpoint(device.id);
            setPlaybackVolume(AudioManager::getPlaybackVolume());
            break;
        }
    }
}

void SoundPanel::onRecordingDeviceChanged(const QString &deviceName) {
    for (const AudioDevice& device : recordingDevices) {
        if (device.shortName == deviceName) {
            AudioManager::setDefaultEndpoint(device.id);
            setRecordingVolume(AudioManager::getRecordingVolume());
            break;
        }
    }
}

void SoundPanel::setOutputButtonImage(int volume) {
    QString icon;
    icon = Utils::getIcon(2, volume, NULL);

    engine->rootContext()->setContextProperty("outputIcon", QStringLiteral("qrc") + icon);
}

void SoundPanel::setInputButtonImage(bool muted) {
    QString icon = Utils::getIcon(3, NULL, muted);

    engine->rootContext()->setContextProperty("inputIcon", QStringLiteral("qrc") + icon);
}

void SoundPanel::onOutputMuteButtonClicked() {
    bool isMuted = AudioManager::getPlaybackMute();
    int volume = AudioManager::getPlaybackVolume();
    AudioManager::setPlaybackMute(!isMuted);

    if (!isMuted || volume == 0) {
        setOutputButtonImage(0);
    } else {
        setOutputButtonImage(volume);
    }

    emit shouldUpdateTray();

}
void SoundPanel::onInputMuteButtonClicked() {
    bool isMuted = AudioManager::getRecordingMute();
    AudioManager::setRecordingMute(!isMuted);
    setInputButtonImage(!isMuted);
}

void SoundPanel::onOutputSliderReleased() {
    if (!systemSoundsMuted) Utils::playSoundNotification();
}

void SoundPanel::populateApplicationModel() {
    applications = AudioManager::enumerateAudioApplications();

    QMap<QString, QVector<Application>> groupedApps;
    QVector<Application> systemSoundApps;
    for (const Application &app : applications) {
        if (app.executableName.isEmpty() ||
            app.executableName.compare("QuickSoundSwitcher", Qt::CaseInsensitive) == 0) {
            continue;
        }

        if (app.name == "Windows system sounds") {
            systemSoundApps.append(app);
            continue;
        }
        groupedApps[app.executableName].append(app);
    }

    QMetaObject::invokeMethod(engine->rootObjects().first(), "clearApplicationModel");

    int addedAppsCount = 0;
    // Process other grouped applications first
    for (auto it = groupedApps.begin(); it != groupedApps.end(); ++it) {
        QString executableName = it.key();
        QVector<Application> appGroup = it.value();

        QString displayName = appGroup.first().name.isEmpty() ? executableName : appGroup.first().name;

        bool isMuted = std::any_of(appGroup.begin(), appGroup.end(), [](const Application &app) {
            return app.isMuted;
        });
        int volume = 0;
        if (!appGroup.isEmpty()) {
            volume = std::max_element(appGroup.begin(), appGroup.end(), [](const Application &a, const Application &b) {
                         return a.volume < b.volume;
                     })->volume;
        }

        QPixmap iconPixmap = appGroup.first().icon.pixmap(16, 16);
        QByteArray byteArray;
        QBuffer buffer(&byteArray);
        buffer.open(QIODevice::WriteOnly);
        iconPixmap.toImage().save(&buffer, "PNG");
        QString base64Icon = QString::fromUtf8(byteArray.toBase64());

        QStringList appIDs;
        for (const Application &app : appGroup) {
            appIDs.append(app.id);
        }

        QMetaObject::invokeMethod(engine->rootObjects().first(), "addApplication",
                                  Q_ARG(QVariant, QVariant::fromValue(appIDs.join(";"))),
                                  Q_ARG(QVariant, QVariant::fromValue(displayName)),
                                  Q_ARG(QVariant, QVariant::fromValue(isMuted)),
                                  Q_ARG(QVariant, QVariant::fromValue(volume)),
                                  Q_ARG(QVariant, QVariant::fromValue(base64Icon)));
        addedAppsCount++;
    }

    // Process system sounds last
    if (!systemSoundApps.isEmpty()) {
        QString displayName = systemSoundApps.first().name.isEmpty() ? "Windows system sounds" : systemSoundApps.first().name;

        bool isMuted = std::any_of(systemSoundApps.begin(), systemSoundApps.end(), [](const Application &app) {
            return app.isMuted;
        });
        int volume = 0;
        if (!systemSoundApps.isEmpty()) {
            volume = std::max_element(systemSoundApps.begin(), systemSoundApps.end(), [](const Application &a, const Application &b) {
                         return a.volume < b.volume;
                     })->volume;
        }

        QPixmap iconPixmap = systemSoundApps.first().icon.pixmap(16, 16);
        QByteArray byteArray;
        QBuffer buffer(&byteArray);
        buffer.open(QIODevice::WriteOnly);
        iconPixmap.toImage().save(&buffer, "PNG");
        QString base64Icon = QString::fromUtf8(byteArray.toBase64());

        QStringList appIDs;
        for (const Application &app : systemSoundApps) {
            appIDs.append(app.id);
        }

        QMetaObject::invokeMethod(engine->rootObjects().first(), "addApplication",
                                  Q_ARG(QVariant, QVariant::fromValue(appIDs.join(";"))),
                                  Q_ARG(QVariant, QVariant::fromValue(displayName)),
                                  Q_ARG(QVariant, QVariant::fromValue(isMuted)),
                                  Q_ARG(QVariant, QVariant::fromValue(volume)),
                                  Q_ARG(QVariant, QVariant::fromValue(base64Icon)));

        systemSoundsMuted = isMuted;
        addedAppsCount++;
    }
    bool singleApp;
    if (addedAppsCount == 1 || mixerOnly) {
        singleApp = true;
    } else {
        singleApp = false;
    }
    engine->rootContext()->setContextProperty("singleApp", singleApp);
}

void SoundPanel::onApplicationVolumeSliderValueChanged(QString appID, int volume) {
    AudioManager::setApplicationVolume(appID, volume);
}

void SoundPanel::onApplicationMuteButtonClicked(QString appID, bool state) {
    if (appID == "0") systemSoundsMuted = state;

    AudioManager::setApplicationMute(appID, state);
}

void SoundPanel::setSystemSoundsIcon() {
    if (Utils::getTheme() == "light") {
        engine->rootContext()->setContextProperty("systemSoundsIcon", QStringLiteral("qrc") + ":/icons/system_light.png");
    } else {
        engine->rootContext()->setContextProperty("systemSoundsIcon", QStringLiteral("qrc") + ":/icons/system_dark.png");
    }
}

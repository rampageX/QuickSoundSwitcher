#include "MediaFlyout.h"
#include "Utils.h"
#include <QQmlContext>
#include <QQuickItem>
#include <QIcon>
#include <QPixmap>
#include <QBuffer>
#include <QApplication>
#include <QTimer>

MediaFlyout::MediaFlyout(QObject* parent)
    : QObject(parent)
    , visible(false)
    , isAnimating(false)
    , mediaFlyoutWindow(nullptr)
{
    engine = new QQmlApplicationEngine(this);
    engine->rootContext()->setContextProperty("mediaFlyout", this);

    QColor windowColor = QGuiApplication::palette().color(QPalette::Window);
    engine->rootContext()->setContextProperty("nativeWindowColor", windowColor);

    QColor accentColor(Utils::getAccentColor("normal"));
    engine->rootContext()->setContextProperty("accentColor", accentColor.name());

    engine->load(QUrl(QStringLiteral("qrc:/qml/MediaFlyout.qml")));

    mediaFlyoutWindow = qobject_cast<QWindow*>(engine->rootObjects().first());
}

MediaFlyout::~MediaFlyout()
{
    delete engine;
}

void MediaFlyout::animateIn()
{
    isAnimating = true;

    QRect availableGeometry = QApplication::primaryScreen()->availableGeometry();

    int panelX = availableGeometry.right() - mediaFlyoutWindow->width() + 1;
    int startY = availableGeometry.bottom() - (mediaFlyoutWindow->height() / 2);
    int targetY = availableGeometry.bottom() - mediaFlyoutWindow->height();

    mediaFlyoutWindow->setPosition(panelX, startY);
    mediaFlyoutWindow->show();

    HWND hwnd = (HWND)mediaFlyoutWindow->winId();
    qDebug() << hwnd;

    const int durationMs = 200;
    const int refreshRate = 1;
    const double totalSteps = durationMs / refreshRate;
    int currentStep = 0;
    QTimer *animationTimer = new QTimer(this);

    animationTimer->start(refreshRate);
    connect(animationTimer, &QTimer::timeout, this, [=]() mutable {
        double t = static_cast<double>(currentStep) / totalSteps;
        double easedT = 1 - pow(2, -3 * t);
        int currentY = startY + easedT * (targetY - startY);

        if (currentY == targetY) {
            animationTimer->stop();
            animationTimer->deleteLater();
            visible = true;
            isAnimating = false;
            return;
        }

        mediaFlyoutWindow->setPosition(panelX, currentY);
        ++currentStep;
    });
}

void MediaFlyout::animateOut()
{
    if (isAnimating || !visible)
        return;

    isAnimating = true;
    mediaFlyoutWindow->setVisible(false);
    visible = false;
    isAnimating = false;
}

int MediaFlyout::playbackVolume() const {
    return m_playbackVolume;
}

void MediaFlyout::setPlaybackVolume(int volume) {
    if (m_playbackVolume != volume) {
        m_playbackVolume = volume;
        AudioManager::setPlaybackVolume(volume);
        emit playbackVolumeChanged();
    }
}

int MediaFlyout::recordingVolume() const {
    return m_recordingVolume;
}

void MediaFlyout::setRecordingVolume(int volume) {
    if (m_recordingVolume != volume) {
        m_recordingVolume = volume;
        emit recordingVolumeChanged();
    }
}

void MediaFlyout::onPlaybackVolumeChanged(int volume) {
    AudioManager::setPlaybackVolume(volume);
    if (AudioManager::getPlaybackMute()) {
        AudioManager::setPlaybackMute(false);
    }
    setOutputButtonImage(volume);
    emit shouldUpdateTray();
}

void MediaFlyout::onRecordingVolumeChanged(int volume) {
    AudioManager::setRecordingVolume(volume);
}

void MediaFlyout::populateComboBoxes() {
    playbackDevices.clear();
    recordingDevices.clear();

    AudioManager::enumeratePlaybackDevices(playbackDevices);
    AudioManager::enumerateRecordingDevices(recordingDevices);

    QMetaObject::invokeMethod(engine->rootObjects().first(), "clearPlaybackDevices");

    int defaultPlaybackIndex = -1;
    for (int i = 0; i < playbackDevices.size(); ++i) {
        const AudioDevice& device = playbackDevices[i];
        QMetaObject::invokeMethod(engine->rootObjects().first(), "addPlaybackDevice",
                                  Q_ARG(QVariant, QVariant::fromValue(device.shortName)));

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

void MediaFlyout::onPlaybackDeviceChanged(const QString &deviceName) {
    for (const AudioDevice& device : playbackDevices) {
        if (device.shortName == deviceName) {
            AudioManager::setDefaultEndpoint(device.id);
            setPlaybackVolume(AudioManager::getPlaybackVolume());
            break;
        }
    }
}

void MediaFlyout::onRecordingDeviceChanged(const QString &deviceName) {
    for (const AudioDevice& device : recordingDevices) {
        if (device.shortName == deviceName) {
            AudioManager::setDefaultEndpoint(device.id);
            setRecordingVolume(AudioManager::getRecordingVolume());
            break;
        }
    }
}

void MediaFlyout::setupUI() {
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

void MediaFlyout::setOutputButtonImage(int volume) {
    QString icon = Utils::getIcon(2, volume, NULL);

    engine->rootContext()->setContextProperty("outputIcon", QStringLiteral("qrc") + icon);
}

void MediaFlyout::setInputButtonImage(bool muted) {
    QString icon = Utils::getIcon(3, NULL, muted);

    engine->rootContext()->setContextProperty("inputIcon", QStringLiteral("qrc") + icon);
}

void MediaFlyout::onOutputMuteButtonClicked() {
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
void MediaFlyout::onInputMuteButtonClicked() {
    bool isMuted = AudioManager::getRecordingMute();
    AudioManager::setRecordingMute(!isMuted);
    setInputButtonImage(!isMuted);
}

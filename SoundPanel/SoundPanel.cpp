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

SoundPanel::SoundPanel(QObject* parent)
    : QObject(parent)
    , soundPanelWindow(nullptr)
{
    engine = new QQmlApplicationEngine(this);
    engine->rootContext()->setContextProperty("soundPanel", this);

    QColor windowColor;
    QColor borderColor;
    if (Utils::getTheme() == "dark") {
        windowColor = QColor(228, 228, 228);
        borderColor = QColor(1, 1, 1, 31);
    } else {
        windowColor = QColor(31, 31, 31);
        borderColor = QColor(255, 255, 255, 31);
    }
    engine->rootContext()->setContextProperty("nativeWindowColor", windowColor);

    QColor accentColor(Utils::getAccentColor("normal"));
    engine->rootContext()->setContextProperty("accentColor", accentColor.name());
    engine->rootContext()->setContextProperty("borderColor", borderColor);

    if (Utils::isWindows10()) {
        engine->load(QUrl(QStringLiteral("qrc:/qml/SoundPanel.qml")));
    } else {
        engine->load(QUrl(QStringLiteral("qrc:/qml/SoundPanel11.qml")));
    }

    soundPanelWindow = qobject_cast<QWindow*>(engine->rootObjects().first());

    setupUI();
    animateIn();

    connect(static_cast<QuickSoundSwitcher*>(parent), &QuickSoundSwitcher::outputMuteStateChanged,
            this, &SoundPanel::onOutputMuteStateChanged);
    connect(static_cast<QuickSoundSwitcher*>(parent), &QuickSoundSwitcher::volumeChangedWithTray,
            this, &SoundPanel::onVolumeChangedWithTray);
}

SoundPanel::~SoundPanel()
{
    delete engine;
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
    QRect availableGeometry = QApplication::primaryScreen()->availableGeometry();

    int margin;
    if (Utils::isWindows10()) {
        margin = 0;
    } else {
        margin = 12;
    }
    int panelX = availableGeometry.right() - soundPanelWindow->width() + 1 - margin;
    int startY = availableGeometry.bottom() - (soundPanelWindow->height() * 80 / 100);
    int targetY = availableGeometry.bottom() - soundPanelWindow->height() - margin;

    soundPanelWindow->setPosition(panelX, startY);
    soundPanelWindow->show();

    const int durationMs = 400;
    const int refreshRate = 1;
    const double totalSteps = durationMs / refreshRate;
    int currentStep = 0;
    QTimer *animationTimer = new QTimer(this);

    animationTimer->start(refreshRate);
    connect(animationTimer, &QTimer::timeout, this, [=, this]() mutable {
        double t = static_cast<double>(currentStep) / totalSteps;
        double easedT = 1 - pow(2, -5 * t);
        int currentY = startY + easedT * (targetY - startY);

        if (currentY == targetY) {
            animationTimer->stop();
            animationTimer->deleteLater();
            return;
        }

        soundPanelWindow->setPosition(panelX, currentY);
        ++currentStep;
    });
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
    qDebug() << volume;
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

void SoundPanel::setupUI() {
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

void SoundPanel::setOutputButtonImage(int volume) {
    QString icon = Utils::getIcon(2, volume, NULL);

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
    Utils::playSoundNotification();
}

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
    , isWindows10(Utils::isWindows10())
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

    setSystemSoundsIcon();

    if (isWindows10) {
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

    QObject *gridLayout = engine->rootObjects().first()->findChild<QObject*>("gridLayout");
    QObject *repeater = gridLayout ? gridLayout->findChild<QObject*>("appRepeater", Qt::FindChildrenRecursively) : nullptr;

    int repeaterSize = 0;
    if (repeater) {
        repeaterSize = 50 * repeater->property("count").toInt();
    }

    int margin;
    if (isWindows10) {
        margin = 0;
    } else {
        margin = 12;
    }

    int panelX = availableGeometry.right() - soundPanelWindow->width() + 1 - margin;

    int startY;
    if (isWindows10) {
        int windowSize = soundPanelWindow->height() + repeaterSize;
        startY = availableGeometry.bottom() - (windowSize * 80 / 100);
    } else {
        startY = availableGeometry.bottom();

    }
    int targetY = availableGeometry.bottom() - soundPanelWindow->height() - repeaterSize - margin;

    soundPanelWindow->setPosition(panelX, startY);
    soundPanelWindow->show();

    // Check if Windows 10 or not to adjust duration and easing
    const int durationMs = isWindows10 ? 400 : 250;
    const int refreshRate = isWindows10 ? 1 : 8;
    const double totalSteps = durationMs / refreshRate;
    int currentStep = 0;
    QTimer *animationTimer = new QTimer(this);

    animationTimer->start(refreshRate);
    connect(animationTimer, &QTimer::timeout, this, [=, this]() mutable {
        double t = static_cast<double>(currentStep) / totalSteps;

        // Use easing function depending on OS
        double easedT;
        if (isWindows10) {
            easedT = 1 - pow(2, -5 * t);  // Exponential easing (for Windows 10)
        } else {
            easedT = 1 - pow(1 - t, 3);
        }

        int currentY = startY + easedT * (targetY - startY);

        if (currentStep == totalSteps) {
            animationTimer->stop();
            animationTimer->deleteLater();
            soundPanelWindow->setPosition(panelX, targetY);

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
    populateApplicationModel();
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

void SoundPanel::populateApplicationModel() {
    applications = AudioManager::enumerateAudioApplications();

    // Sort applications to put "Windows system sounds" first
    std::sort(applications.begin(), applications.end(), [](const Application &a, const Application &b) {
        if (a.name == "Windows system sounds") return true;
        if (b.name == "Windows system sounds") return false;
        return a.name < b.name;  // Default sorting for other applications
    });

    QMetaObject::invokeMethod(engine->rootObjects().first(), "clearApplicationModel");

    for (const Application &app : applications) {
        // Use app.name if not empty, otherwise use app.executableName
        QString displayName = app.name.isEmpty() ? app.executableName : app.name;

        // Convert QPixmap to QByteArray (Base64 encoding)
        QPixmap iconPixmap = app.icon.pixmap(16, 16);
        QByteArray byteArray;
        QBuffer buffer(&byteArray);
        buffer.open(QIODevice::WriteOnly);
        iconPixmap.toImage().save(&buffer, "PNG");  // Save as PNG to the buffer

        QString base64Icon = QString::fromUtf8(byteArray.toBase64());

        // Pass the base64 string and display name to QML
        QMetaObject::invokeMethod(engine->rootObjects().first(), "addApplication",
                                  Q_ARG(QVariant, QVariant::fromValue(app.id)),
                                  Q_ARG(QVariant, QVariant::fromValue(displayName)),
                                  Q_ARG(QVariant, QVariant::fromValue(app.isMuted)),
                                  Q_ARG(QVariant, QVariant::fromValue(app.volume)),
                                  Q_ARG(QVariant, QVariant::fromValue(base64Icon)));
    }
}

void SoundPanel::onApplicationVolumeSliderValueChanged(QString appID, int volume) {
    AudioManager::setApplicationVolume(appID, volume);
}

void SoundPanel::onApplicationMuteButtonClicked(QString appID, bool state) {
    AudioManager::setApplicationMute(appID, state);
}

void SoundPanel::setSystemSoundsIcon() {
    if (Utils::getTheme() == "light") {
        engine->rootContext()->setContextProperty("systemSoundsIcon", QStringLiteral("qrc") + ":/icons/system_light.png");
    } else {
        engine->rootContext()->setContextProperty("systemSoundsIcon", QStringLiteral("qrc") + ":/icons/system_dark.png");
    }
}

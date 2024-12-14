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
    , isWindows10(Utils::isWindows10())
{
    engine = new QQmlApplicationEngine(this);

    QColor windowColor;
    QColor borderColor;
    QColor contrastedColor;
    QColor contrastedBorderColor;
    if (Utils::getTheme() == "dark") {
        windowColor = QColor(242, 242, 242);
        contrastedColor = QColor(238,238,238);
        borderColor = QColor(1, 1, 1, 31);
        contrastedBorderColor = QColor(224, 224, 224);
    } else {
        windowColor = QColor(36, 36, 36);
        contrastedColor = QColor(30 ,30 ,30 );
        borderColor = QColor(255, 255, 255, 31);
        contrastedBorderColor = QColor(25, 25, 25);
    }

    QColor accentColor(Utils::getAccentColor("normal"));

    engine->rootContext()->setContextProperty("soundPanel", this);
    engine->rootContext()->setContextProperty("accentColor", accentColor.name());
    engine->rootContext()->setContextProperty("borderColor", borderColor);
    engine->rootContext()->setContextProperty("nativeWindowColor", windowColor);
    engine->rootContext()->setContextProperty("contrastedColor", contrastedColor);
    engine->rootContext()->setContextProperty("contrastedBorderColor", contrastedBorderColor);

    setSystemSoundsIcon();

    QString uiFile = isWindows10 ? "qrc:/qml/SoundPanel.qml" : "qrc:/qml/SoundPanel11.qml";
    engine->load(QUrl(uiFile));

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

    int margin = isWindows10 ? margin = -1 : margin = 12;
    int windowHeight = soundPanelWindow->height() + repeaterSize;
    int panelX = availableGeometry.right() - soundPanelWindow->width() + 1 - margin;

    int scalingFactor = isWindows10 ? 80 : 50;
    int startY = availableGeometry.bottom() - (windowHeight * scalingFactor / 100);

    int targetY = availableGeometry.bottom() - windowHeight + 1;
    soundPanelWindow->setPosition(panelX, startY);

    QPropertyAnimation *animation = new QPropertyAnimation(soundPanelWindow, "y", this);
    animation->setDuration(isWindows10 ? 300 : 200);
    animation->setStartValue(startY);
    animation->setEndValue(targetY);
    animation->setEasingCurve(QEasingCurve::OutQuad);

    connect(animation, &QPropertyAnimation::finished, animation, &QObject::deleteLater);

    soundPanelWindow->show();
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
    QString icon;
    if (isWindows10) {
        icon = Utils::getIcon(2, volume, NULL);
    } else {
        icon = Utils::getIcon(1, volume, NULL);
    }

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

    // Group applications by executable name
    QMap<QString, QVector<Application>> groupedApps;
    for (const Application &app : applications) {
        if (app.executableName.compare("QuickSoundSwitcher", Qt::CaseInsensitive) == 0) {
            continue;
        }
        groupedApps[app.executableName].append(app);
    }

    QMetaObject::invokeMethod(engine->rootObjects().first(), "clearApplicationModel");

    for (auto it = groupedApps.begin(); it != groupedApps.end(); ++it) {
        QString executableName = it.key();
        QVector<Application> appGroup = it.value();

        QString displayName = appGroup.first().name.isEmpty() ? executableName : appGroup.first().name;

        // Aggregate mute and volume states
        bool isMuted = std::any_of(appGroup.begin(), appGroup.end(), [](const Application &app) {
            return app.isMuted;
        });
        int volume = 0;
        if (!appGroup.isEmpty()) {
            volume = std::max_element(appGroup.begin(), appGroup.end(), [](const Application &a, const Application &b) {
                         return a.volume < b.volume;
                     })->volume;
        }

        // Convert QPixmap to Base64 string
        QPixmap iconPixmap = appGroup.first().icon.pixmap(16, 16);
        QByteArray byteArray;
        QBuffer buffer(&byteArray);
        buffer.open(QIODevice::WriteOnly);
        iconPixmap.toImage().save(&buffer, "PNG");
        QString base64Icon = QString::fromUtf8(byteArray.toBase64());

        // Store appIDs for grouped control
        QStringList appIDs;
        for (const Application &app : appGroup) {
            appIDs.append(app.id);
        }

        // Add the grouped entry to QML
        QMetaObject::invokeMethod(engine->rootObjects().first(), "addApplication",
                                  Q_ARG(QVariant, QVariant::fromValue(appIDs.join(";"))),
                                  Q_ARG(QVariant, QVariant::fromValue(displayName)),
                                  Q_ARG(QVariant, QVariant::fromValue(isMuted)),
                                  Q_ARG(QVariant, QVariant::fromValue(volume)),
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

#include "Panel.h"
#include "ui_Panel.h"
#include "AudioManager.h"
#include "Utils.h"
#include <QComboBox>
#include <QList>
#include <QProcess>
#include <QRegularExpression>
#include <QStringList>
#include <QPropertyAnimation>
#include <QScreen>
#include <QGuiApplication>
#include <QPainter>
#include <QTimer>
#include <QPainterPath>

Panel::Panel(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Panel)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedWidth(width());
    this->installEventFilter(this);
    AudioManager::initialize();
    populateComboBoxes();
    populateApplications();
    setSliders();
    setButtons();
    Utils::setFrameColorBasedOnWindow(this, ui->appFrame);
    Utils::setFrameColorBasedOnWindow(this, ui->inputFrame);
    Utils::setFrameColorBasedOnWindow(this, ui->outputFrame);

    QTimer *audioMeterTimer = new QTimer(this);
    connect(audioMeterTimer, &QTimer::timeout, this, &Panel::outputAudioMeter);
    connect(audioMeterTimer, &QTimer::timeout, this, &Panel::inputAudioMeter);
    audioMeterTimer->start(32);

    connect(ui->outputComboBox, &QComboBox::currentIndexChanged, this, &Panel::onOutputComboBoxIndexChanged);
    connect(ui->inputComboBox, &QComboBox::currentIndexChanged, this, &Panel::onInputComboBoxIndexChanged);
    connect(ui->outputVolumeSlider, &QSlider::valueChanged, this, &Panel::onOutputValueChanged);
    connect(ui->inputVolumeSlider, &QSlider::valueChanged, this, &Panel::onInputValueChanged);
    connect(ui->outputMuteButton, &QPushButton::pressed, this, &Panel::onOutputMuteButtonPressed);
    connect(ui->inputMuteButton, &QPushButton::pressed, this, &Panel::onInputMuteButtonPressed);
}

Panel::~Panel()
{
    AudioManager::cleanup();
    delete ui;
}

void Panel::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);  // Prevent unused parameter warning
    QPainter painter(this);

    // Get the main background color from the widget's palette
    QColor main_bg_color = this->palette().color(QPalette::Window);

    // Set the brush to fill the rectangle with the background color
    painter.setBrush(main_bg_color);
    painter.setPen(Qt::NoPen); // No border for background fill

    // Create a rounded rectangle path
    QPainterPath path;
    path.addRoundedRect(this->rect().adjusted(1, 1, -1, -1), 8, 8); // Adjusted for inner fill

    // Draw the filled rounded rectangle
    painter.drawPath(path);

    // Now set the pen for the border with 30% alpha
    QColor borderColor = QColor(255, 255, 255, 32); // White with 30% alpha (255 * 0.3 = 77)
    QPen borderPen(borderColor);
    borderPen.setWidth(1);
    painter.setPen(borderPen);
    painter.setBrush(Qt::NoBrush); // No fill for border

    // Draw the border around the adjusted rectangle
    QPainterPath borderPath;
    borderPath.addRoundedRect(this->rect().adjusted(0, 0, -1, -1), 8, 8); // Full size for outer border
    painter.drawPath(borderPath);
}

void Panel::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    raise();
    activateWindow();
}

void Panel::closeEvent(QCloseEvent *event)
{
    event->ignore();
}

void Panel::populateComboBoxes()
{
    AudioManager::enumeratePlaybackDevices(playbackDevices);
    AudioManager::enumerateRecordingDevices(recordingDevices);

    ui->outputComboBox->clear();
    ui->inputComboBox->clear();

    int defaultPlaybackIndex = -1;
    int defaultRecordingIndex = -1;

    for (const AudioDevice &device : playbackDevices) {
        ui->outputComboBox->addItem(device.shortName);

        if (device.isDefault) {
            defaultPlaybackIndex = ui->outputComboBox->count() - 1;
        }
    }

    if (defaultPlaybackIndex != -1) {
        ui->outputComboBox->setCurrentIndex(defaultPlaybackIndex);
    }

    for (const AudioDevice &device : recordingDevices) {
        ui->inputComboBox->addItem(device.shortName);

        if (device.isDefault) {
            defaultRecordingIndex = ui->inputComboBox->count() - 1;
        }
    }

    if (defaultRecordingIndex != -1) {
        ui->inputComboBox->setCurrentIndex(defaultRecordingIndex);
    }
}

void Panel::setSliders()
{
    ui->outputVolumeSlider->setValue(AudioManager::getPlaybackVolume());
    ui->inputVolumeSlider->setValue(AudioManager::getRecordingVolume());
    ui->outputVolumeSlider->setEnabled(!AudioManager::getPlaybackMute());
    ui->inputVolumeSlider->setEnabled(!AudioManager::getRecordingMute());
}

void Panel::setButtons()
{
    ui->outputMuteButton->setIcon(Utils::getIcon(2, NULL, AudioManager::getPlaybackMute()));
    ui->outputMuteButton->setIconSize(QSize(16, 16));
    ui->inputMuteButton->setIcon(Utils::getIcon(3, NULL, AudioManager::getRecordingMute()));
    ui->inputMuteButton->setIconSize(QSize(16, 16));
}

void Panel::setFrames()
{
    Utils::setFrameColorBasedOnWindow(this, ui->outputFrame);
    Utils::setFrameColorBasedOnWindow(this, ui->inputFrame);
    Utils::setFrameColorBasedOnWindow(this, ui->appFrame);
}

void Panel::onOutputComboBoxIndexChanged(int index)
{
    if (index < 0 || index >= playbackDevices.size()) {
        return;
    }

    const AudioDevice &selectedDevice = playbackDevices[index];
    AudioManager::setDefaultEndpoint(selectedDevice.id);

    updateUi();
}

void Panel::onInputComboBoxIndexChanged(int index)
{
    if (index < 0 || index >= recordingDevices.size()) {
        return;
    }

    const AudioDevice &selectedDevice = recordingDevices[index];
    AudioManager::setDefaultEndpoint(selectedDevice.id);

    updateUi();
}

void Panel::onOutputValueChanged()
{
    AudioManager::setPlaybackVolume(ui->outputVolumeSlider->value());
    emit volumeChanged();
}

void Panel::onInputValueChanged()
{
    AudioManager::setRecordingVolume(ui->inputVolumeSlider->value());
}

void Panel::onOutputMuteButtonPressed()
{
    bool playbackMute = AudioManager::getPlaybackMute();
    AudioManager::setPlaybackMute(!playbackMute);
    ui->outputVolumeSlider->setEnabled(playbackMute);
    ui->outputMuteButton->setIcon(Utils::getIcon(2, NULL, !playbackMute));
    emit outputMuteChanged();
}

void Panel::onInputMuteButtonPressed()
{
    bool recordingMute = AudioManager::getRecordingMute();
    AudioManager::setRecordingMute(!recordingMute);
    ui->inputVolumeSlider->setEnabled(recordingMute);
    ui->inputMuteButton->setIcon(Utils::getIcon(3, NULL, !recordingMute));
}

void Panel::outputAudioMeter() {
    int level = AudioManager::getPlaybackAudioLevel();
    ui->outputAudioMeter->setValue(level);
}

void Panel::inputAudioMeter() {
    int level = AudioManager::getRecordingAudioLevel();
    ui->inputAudioMeter->setValue(level);
}

bool Panel::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        if (!this->underMouse()) {
            emit lostFocus();
        }
    }

    return QWidget::eventFilter(obj, event);
}

void Panel::updateUi()
{
    ui->outputVolumeSlider->setValue(AudioManager::getPlaybackVolume());
    ui->inputVolumeSlider->setValue(AudioManager::getRecordingVolume());

    bool playbackMute = AudioManager::getPlaybackMute();
    bool recordingMute = AudioManager::getRecordingMute();

    ui->outputMuteButton->setIcon(Utils::getIcon(2, NULL, playbackMute));
    ui->inputMuteButton->setIcon(Utils::getIcon(3, NULL, recordingMute));

    ui->outputVolumeSlider->setEnabled(!playbackMute);
    ui->inputVolumeSlider->setEnabled(!recordingMute);
}

void Panel::populateApplications()
{
    if (!ui->appFrame->layout()) {
        ui->appFrame->setLayout(new QVBoxLayout);
    }

    QVBoxLayout *vBoxLayout = qobject_cast<QVBoxLayout *>(ui->appFrame->layout());
    QList<Application> applications = AudioManager::enumerateAudioApplications();

    int marginLeft, marginTop, marginRight, marginBottom;
    vBoxLayout->getContentsMargins(&marginLeft, &marginTop, &marginRight, &marginBottom);
    int spacing = vBoxLayout->spacing();
    int totalHeight = marginTop + marginBottom;

    bool shouldDisplayLabel = applications.isEmpty() || (applications.size() == 1 && applications[0].name == "@%SystemRoot%\\System32\\AudioSrv.Dll,-202");

    if (shouldDisplayLabel) {
        QLabel *label = new QLabel(tr("All quiet for now."), ui->appFrame);
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("font-style: italic;");
        vBoxLayout->addWidget(label);

        totalHeight += label->sizeHint().height() + spacing;
    } else {
        for (const Application& app : applications) {
            if (app.name == "@%SystemRoot%\\System32\\AudioSrv.Dll,-202") {
                continue;
            }

            QHBoxLayout *hBoxLayout = new QHBoxLayout;

            QPushButton *muteButton = new QPushButton(ui->appFrame);
            muteButton->setFixedSize(35, 35);
            muteButton->setToolTip(app.executableName);

            QIcon originalIcon = app.icon;
            QPixmap originalPixmap = originalIcon.pixmap(16, 16);

            if (app.isMuted) {
                QIcon mutedIcon = Utils::generateMutedIcon(originalPixmap);
                muteButton->setIcon(mutedIcon);
            } else {
                muteButton->setIcon(originalIcon);
            }

            connect(muteButton, &QPushButton::clicked, [appId = app.id, muteButton, originalPixmap]() mutable {
                bool newMuteState = !AudioManager::getApplicationMute(appId);
                AudioManager::setApplicationMute(appId, newMuteState);

                if (newMuteState) {
                    QIcon mutedIcon = Utils::generateMutedIcon(originalPixmap);
                    muteButton->setIcon(mutedIcon);
                } else {
                    muteButton->setIcon(QIcon(originalPixmap));
                }
            });

            QSlider *slider = new QSlider(Qt::Horizontal, ui->appFrame);
            slider->setRange(0, 100);
            slider->setValue(app.volume);

            connect(slider, &QSlider::valueChanged, [appId = app.id](int value) {
                AudioManager::setApplicationVolume(appId, value);
            });

            hBoxLayout->addWidget(muteButton);
            hBoxLayout->addWidget(slider);
            vBoxLayout->addLayout(hBoxLayout);

            totalHeight += muteButton->height() + spacing;
        }
    }

    // Adjust total height by subtracting the last spacing (no spacing after the last row)
    totalHeight -= spacing;

    ui->appFrame->setMinimumHeight(totalHeight);
    setMinimumHeight(height() + 12 + ui->appFrame->height());
}

void Panel::fadeIn()
{
    QPropertyAnimation *animation = new QPropertyAnimation(this, "windowOpacity");
    animation->setDuration(200);
    animation->setStartValue(0);
    animation->setEndValue(1);
    setWindowOpacity(0);
    show();
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void Panel::fadeOut()
{
    QPropertyAnimation *animation = new QPropertyAnimation(this, "windowOpacity");
    animation->setDuration(200);
    animation->setStartValue(1);
    animation->setEndValue(0);

    connect(animation, &QPropertyAnimation::finished, this, [this]() {
        emit fadeOutFinished();
    });

    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

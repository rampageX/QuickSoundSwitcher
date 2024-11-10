#include "Panel.h"
#include "ui_Panel.h"
#include "AudioManager.h"
#include "Utils.h"
#include <QComboBox>
#include <QDebug>
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
    setFixedSize(size());
    this->installEventFilter(this);

    QRect screenGeometry = QGuiApplication::primaryScreen()->availableGeometry();
    QPoint screenCenter = screenGeometry.bottomLeft();
    move(screenCenter.x() - width() / 2, screenCenter.y());
    AudioManager::initialize();
    populateComboBoxes();
    populateApplications();

    setSliders();
    setButtons();
    setFrames();

    QTimer *audioMeterTimer = new QTimer(this);
    connect(audioMeterTimer, &QTimer::timeout, this, &Panel::outputAudioMeter);
    connect(audioMeterTimer, &QTimer::timeout, this, &Panel::inputAudioMeter);
    audioMeterTimer->start(32);

    connect(ui->outputComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &Panel::onOutputComboBoxIndexChanged);
    connect(ui->inputComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &Panel::onInputComboBoxIndexChanged);

    connect(ui->outputVolumeSlider, &QSlider::valueChanged, this, &Panel::onOutputValueChanged);
    connect(ui->inputVolumeSlider, &QSlider::valueChanged, this, &Panel::onInputValueChanged);
    connect(ui->outputMuteButton, &QToolButton::pressed, this, &Panel::onOutputMuteButtonPressed);
    connect(ui->inputMuteButton, &QToolButton::pressed, this, &Panel::onInputMuteButtonPressed);
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
    painter.setPen(Qt::NoPen); // No border

    // Create a rounded rectangle path
    QPainterPath path;
    path.addRoundedRect(this->rect(), 8, 8); // 8px radius

    // Draw the rounded rectangle
    painter.drawPath(path);
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
    ui->outputMuteButton->setIconSize(QSize(16, 16));
    emit outputMuteChanged();
}

void Panel::onInputMuteButtonPressed()
{
    bool recordingMute = AudioManager::getRecordingMute();
    AudioManager::setRecordingMute(!recordingMute);
    ui->inputVolumeSlider->setEnabled(recordingMute);
    ui->inputMuteButton->setIcon(Utils::getIcon(3, NULL, !recordingMute));
    ui->inputMuteButton->setIconSize(QSize(16, 16));
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

void Panel::populateApplications() {
    // Ensure the frame's layout exists and is a QGridLayout
    if (!ui->appFrame->layout()) {
        ui->appFrame->setLayout(new QGridLayout);
    }

    QGridLayout *gridLayout = qobject_cast<QGridLayout *>(ui->appFrame->layout());
    if (!gridLayout) {
        qWarning() << "ui->appFrame layout is not a QGridLayout!";
        return;
    }

    // Clear existing widgets in the frame to avoid duplicates
    while (QLayoutItem *child = gridLayout->takeAt(0)) {
        delete child->widget();
        delete child;
    }

    // Retrieve the list of active audio applications
    QList<Application> applications = AudioManager::enumerateAudioApplications();

    int row = 0;  // Track the current row for placing widgets
    QList<QSlider*> sliders;
    QList<QToolButton*> muteButtons;

    // Create sliders and mute buttons for each application
    for (const Application& app : applications) {
        // Skip the application with the specific name
        if (app.name == "@%SystemRoot%\\System32\\AudioSrv.Dll,-202") {
            continue;  // Skip this iteration and move to the next application
        }

        // Create a vertical slider for volume control
        QSlider *slider = new QSlider(Qt::Vertical, ui->appFrame);
        slider->setRange(0, 100);  // Volume range from 0 to 100
        slider->setValue(app.volume);  // Set current volume level

        // Create a tool button for mute/unmute control
        QToolButton *muteButton = new QToolButton(ui->appFrame);
        muteButton->setFixedSize(30, 30);
        muteButton->setToolButtonStyle(Qt::ToolButtonIconOnly);

        // Scale the application icon to 16x16
        QIcon originalIcon = app.icon;
        QPixmap originalPixmap = originalIcon.pixmap(16, 16);

        // Set the icon to muted or original depending on the mute state
        if (app.isMuted) {
            QIcon mutedIcon = Utils::generateMutedIcon(originalPixmap);
            muteButton->setIcon(mutedIcon);
        } else {
            muteButton->setIcon(originalIcon);
        }

        // Connect slider to control application's volume
        connect(slider, &QSlider::valueChanged, [appId = app.id](int value) {
            AudioManager::setApplicationVolume(appId, value);
        });

        // Connect muteButton to toggle mute for the application
        connect(muteButton, &QToolButton::clicked, [appId = app.id, muteButton, originalPixmap]() mutable {
            bool newMuteState = !AudioManager::getApplicationMute(appId);
            AudioManager::setApplicationMute(appId, newMuteState);

            if (newMuteState) {
                QIcon mutedIcon = Utils::generateMutedIcon(originalPixmap);
                muteButton->setIcon(mutedIcon);
            } else {
                muteButton->setIcon(QIcon(originalPixmap));
            }

            muteButton->setText(newMuteState ? "Unmute" : "Mute");
        });

        // Place widgets in the grid layout
        gridLayout->addWidget(slider, 1, row, Qt::AlignHCenter);     // Slider on the second row
        gridLayout->addWidget(muteButton, 2, row, Qt::AlignHCenter); // Mute button with icon on the third row

        sliders.append(slider);
        muteButtons.append(muteButton);
        ++row;  // Move to the next column for the next application
    }

    // If there are less than 7 applications, create additional sliders and mute buttons
    while (row < 7) {
        // Create empty slider and mute button
        QSlider *slider = new QSlider(Qt::Vertical, ui->appFrame);
        slider->setRange(0, 100);  // Volume range from 0 to 100
        slider->setValue(0);  // Default value (muted)
        slider->setEnabled(false);  // Disable slider

        QToolButton *muteButton = new QToolButton(ui->appFrame);
        muteButton->setFixedSize(30, 30);
        muteButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
        muteButton->setEnabled(false);  // Disable mute button

        // Add to the grid layout
        gridLayout->addWidget(slider, 1, row, Qt::AlignHCenter);
        gridLayout->addWidget(muteButton, 2, row, Qt::AlignHCenter);

        sliders.append(slider);
        muteButtons.append(muteButton);
        ++row;  // Move to the next column
    }
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

#include "panel.h"
#include "ui_panel.h"
#include "audiomanager.h"
#include "utils.h"
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

using namespace AudioManager;
using namespace Utils;

Panel::Panel(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Panel)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    setFixedSize(size());

    QRect screenGeometry = QGuiApplication::primaryScreen()->availableGeometry();
    QPoint screenCenter = screenGeometry.bottomLeft();
    move(screenCenter.x() - width() / 2, screenCenter.y());
    initialize(); // init audiomanager
    populateComboBoxes();
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
    cleanup(); //clean audiomanager
    delete ui;
}

void Panel::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);

    raise();
    activateWindow();
}

void Panel::closeEvent(QCloseEvent *event) {
    event->ignore();
}

void Panel::populateComboBoxes()
{
    enumeratePlaybackDevices(playbackDevices);
    enumerateRecordingDevices(recordingDevices);

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

void Panel::setSliders() {
    ui->outputVolumeSlider->setValue(getPlaybackVolume());
    ui->inputVolumeSlider->setValue(getRecordingVolume());
}

void Panel::setButtons() {
    ui->outputMuteButton->setIcon(getIcon(2, NULL, getPlaybackMute()));
    ui->outputMuteButton->setIconSize(QSize(16, 16));
    ui->inputMuteButton->setIcon(getIcon(3, NULL, getRecordingMute()));
    ui->inputMuteButton->setIconSize(QSize(16, 16));
}

void Panel::setFrames() {
    setFrameColorBasedOnWindow(this, ui->outputFrame);
    setFrameColorBasedOnWindow(this, ui->inputFrame);
}

void Panel::setAudioDevice(const QString& deviceId)
{
    QString command = QString("Set-AudioDevice -ID \"%1\"").arg(deviceId); // Use escaped double quotes

    QProcess process;
    process.start("powershell.exe", QStringList() << "-Command" << command);

    if (!process.waitForFinished()) {
        qDebug() << "Error executing PowerShell command:" << process.errorString();
    }
}

void Panel::onOutputComboBoxIndexChanged(int index)
{
    if (index < 0 || index >= playbackDevices.size()) {
        return;
    }

    const AudioDevice &selectedDevice = playbackDevices[index];
    setAudioDevice(selectedDevice.id);
}

void Panel::onInputComboBoxIndexChanged(int index)
{
    if (index < 0 || index >= recordingDevices.size()) {
        return;
    }

    const AudioDevice &selectedDevice = recordingDevices[index];
    setAudioDevice(selectedDevice.id);
}

void Panel::onOutputValueChanged(int value)
{
        setPlaybackVolume(ui->outputVolumeSlider->value());
        emit volumeChanged();
}

void Panel::onInputValueChanged(int value)
{
    setRecordingVolume(ui->inputVolumeSlider->value());
}

void Panel::onOutputMuteButtonPressed()
{
    bool playbackMute = getPlaybackMute();
    if (playbackMute) {
        setPlaybackMute(false);
    } else {
        setPlaybackMute(true);
    }
    ui->outputMuteButton->setIcon(getIcon(2, NULL, !playbackMute));
    ui->outputMuteButton->setIconSize(QSize(16, 16));
}

void Panel::onInputMuteButtonPressed()
{
    bool recordingMute = getRecordingMute();
    if (recordingMute) {
        setRecordingMute(false);
    } else {
        setRecordingMute(true);
    }
    ui->inputMuteButton->setIcon(getIcon(3, NULL, !recordingMute));
    ui->inputMuteButton->setIconSize(QSize(16, 16));
}

void Panel::outputAudioMeter() {
    int level = getPlaybackAudioLevel();
    ui->outputAudioMeter->setValue(level);
}

void Panel::inputAudioMeter() {
    int level = getRecordingAudioLevel();
    ui->inputAudioMeter->setValue(level);
}

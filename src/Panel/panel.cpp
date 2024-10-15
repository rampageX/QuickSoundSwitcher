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

using namespace AudioManager;
using namespace Utils;

Panel::Panel(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Panel)
    , userClicked(false)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Popup);
    setFixedSize(size());

    QRect screenGeometry = QGuiApplication::primaryScreen()->availableGeometry();
    QPoint screenCenter = screenGeometry.bottomLeft();
    move(screenCenter.x() - width() / 2, screenCenter.y());
    populateComboBoxes();
    setSliders();
    setButtons();
    setFrames();

    connect(ui->outputComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &Panel::onOutputComboBoxIndexChanged);
    connect(ui->inputComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &Panel::onInputComboBoxIndexChanged);

    connect(ui->outputVolumeSlider, &QSlider::valueChanged, this, &Panel::onOutputValueChanged);
    connect(ui->outputVolumeSlider, &QSlider::sliderPressed, this, &Panel::onOutputSliderPressed);
    connect(ui->outputVolumeSlider, &QSlider::sliderReleased, this, &Panel::onOutputSliderReleased);
    connect(ui->inputVolumeSlider, &QSlider::valueChanged, this, &Panel::onInputValueChanged);
    connect(ui->inputVolumeSlider, &QSlider::sliderPressed, this, &Panel::onInputSliderPressed);
    connect(ui->inputVolumeSlider, &QSlider::sliderReleased, this, &Panel::onInputSliderReleased);
    connect(ui->outputMuteButton, &QToolButton::pressed, this, &Panel::onOutputMuteButtonPressed);
    connect(ui->inputMuteButton, &QToolButton::pressed, this, &Panel::onInputMuteButtonPressed);
}

Panel::~Panel()
{
    emit closed();
    delete ui;
}

void Panel::showEvent(QShowEvent *event) {
    QMainWindow::showEvent(event);

    animatePanelIn();
}

void Panel::closeEvent(QCloseEvent *event) {
    event->ignore();
    animatePanelOut();
    raise();
    activateWindow();
}

void Panel::animatePanelIn() {
    QPropertyAnimation *animation = new QPropertyAnimation(this, "pos");
    animation->setDuration(100);
    animation->setStartValue(QPoint(x(), y() + height()));
    animation->setEndValue(QPoint(x(), y()));
    animation->setEasingCurve(QEasingCurve::Linear);

    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void Panel::animatePanelOut() {
    QPropertyAnimation *animation = new QPropertyAnimation(this, "pos");
    animation->setDuration(100);
    animation->setStartValue(QPoint(x(), y()));
    animation->setEndValue(QPoint(x(), y() + height()));
    animation->setEasingCurve(QEasingCurve::Linear);
    connect(animation, &QPropertyAnimation::finished, this, &Panel::onAnimationFinished);

    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void Panel::onAnimationFinished() {
    this->deleteLater();
}

void Panel::populateComboBoxes()
{
    parseAudioDeviceOutput(playbackDevices, recordingDevices);

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
    ui->outputVolumeSlider->setValue(getVolume(true));
    ui->inputVolumeSlider->setValue(getVolume(false));
}

void Panel::setButtons() {
    ui->outputMuteButton->setIcon(getIcon(2, NULL, getMute(true)));
    ui->outputMuteButton->setIconSize(QSize(16, 16));
    ui->inputMuteButton->setIcon(getIcon(3, NULL, getMute(false)));
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

void Panel::onOutputSliderPressed()
{
    userClicked = true;
}

void Panel::onOutputValueChanged(int value)
{
    if (userClicked) {
        return;
    } else {
        setVolume(ui->outputVolumeSlider->value(), true);
        emit volumeChanged();
    }
}

void Panel::onOutputSliderReleased()
{
    userClicked = false;
    setVolume(ui->outputVolumeSlider->value(), true);
    emit volumeChanged();
}

void Panel::onInputSliderPressed()
{
    userClicked = true;
}

void Panel::onInputValueChanged(int value)
{
    if (userClicked) {
        return;
    } else {
        setVolume(ui->inputVolumeSlider->value(), false);
    }
}

void Panel::onInputSliderReleased()
{
    userClicked = false;
    setVolume(ui->inputVolumeSlider->value(), false);
}

void Panel::onOutputMuteButtonPressed()
{
    setMute(true);
    ui->outputMuteButton->setIcon(getIcon(2, NULL, getMute(true)));
    ui->outputMuteButton->setIconSize(QSize(16, 16));
}

void Panel::onInputMuteButtonPressed()
{
    setMute(false);
    ui->inputMuteButton->setIcon(getIcon(3, NULL, getMute(false)));
    ui->inputMuteButton->setIconSize(QSize(16, 16));
}

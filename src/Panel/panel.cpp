#include "panel.h"
#include "ui_panel.h"
#include "audiomanager.h"
#include <QComboBox>
#include <QDebug>
#include <QList>
#include <QProcess>
#include <QRegularExpression>
#include <QStringList>
#include <QPropertyAnimation>
#include <QScreen>
#include <QGuiApplication> // Include for QGuiApplication

using namespace AudioManager;

Panel::Panel(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Panel)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Popup);
    setFixedSize(size());

    // Set the initial position off-screen
    QRect screenGeometry = QGuiApplication::primaryScreen()->availableGeometry();
    QPoint screenCenter = screenGeometry.bottomLeft();
    move(screenCenter.x() - width() / 2, screenCenter.y()); // Adjust to position off-screen

    populateComboBoxes();

    connect(ui->outputComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &Panel::onOutputComboBoxIndexChanged);
    connect(ui->inputComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &Panel::onInputComboBoxIndexChanged);
}

Panel::~Panel()
{
    emit closed();
    delete ui;
}

void Panel::showEvent(QShowEvent *event) {
    QMainWindow::showEvent(event); // Call base class implementation

    // Start animation when the panel is shown
    animatePanelIn();
}

void Panel::closeEvent(QCloseEvent *event) {
    // Prevent the widget from closing immediately
    event->ignore(); // Ignore the close event to allow for animation
    animatePanelOut(); // Start the close animation
}

void Panel::animatePanelIn() {
    // Create the animation for sliding in
    QPropertyAnimation *animation = new QPropertyAnimation(this, "pos");
    animation->setDuration(100); // Duration in milliseconds
    animation->setStartValue(QPoint(x(), y() + height())); // Start below the screen
    animation->setEndValue(QPoint(x(), y())); // End at the current position
    animation->setEasingCurve(QEasingCurve::Linear); // Smooth easing

    animation->start(QAbstractAnimation::DeleteWhenStopped); // Automatically delete the animation when done
}

void Panel::animatePanelOut() {
    // Create the animation for sliding out
    QPropertyAnimation *animation = new QPropertyAnimation(this, "pos");
    animation->setDuration(100); // Duration in milliseconds
    animation->setStartValue(QPoint(x(), y())); // Start at the current position
    animation->setEndValue(QPoint(x(), y() + height())); // End below the screen
    animation->setEasingCurve(QEasingCurve::Linear);
    // Connect to the finished signal to close the widget after the animation
    connect(animation, &QPropertyAnimation::finished, this, &Panel::onAnimationFinished);

    animation->start(QAbstractAnimation::DeleteWhenStopped); // Automatically delete the animation when done
}

void Panel::onAnimationFinished() {
    // This slot is called after the animation finishes
    qDebug() << "finished";
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
        qDebug() << "Output:" << device.shortName;
        qDebug() << "Is default:" << device.isDefault;

        if (device.isDefault) {
            defaultPlaybackIndex = ui->outputComboBox->count() - 1;
        }
    }

    if (defaultPlaybackIndex != -1) {
        ui->outputComboBox->setCurrentIndex(defaultPlaybackIndex);
    }

    for (const AudioDevice &device : recordingDevices) {
        ui->inputComboBox->addItem(device.shortName);
        qDebug() << "Input:" << device.shortName;
        qDebug() << "Is default:" << device.isDefault;

        if (device.isDefault) {
            defaultRecordingIndex = ui->inputComboBox->count() - 1;
        }
    }

    if (defaultRecordingIndex != -1) {
        ui->inputComboBox->setCurrentIndex(defaultRecordingIndex);
    }
}

void Panel::setAudioDevice(const QString& deviceId)
{
    QString command = QString("Set-AudioDevice -ID \"%1\"").arg(deviceId); // Use escaped double quotes

    QProcess process;
    process.start("powershell.exe", QStringList() << "-Command" << command);
    qDebug() << command;

    if (!process.waitForFinished()) {
        qDebug() << "Error executing PowerShell command:" << process.errorString();
    } else {
        qDebug() << "Audio device changed to ID:" << deviceId;
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

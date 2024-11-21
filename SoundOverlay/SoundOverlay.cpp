#include "SoundOverlay.h"
#include "ui_SoundOverlay.h"
#include "Utils.h"
#include "AudioManager.h"
#include <QTimer>
#include <QApplication>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>


SoundOverlay::SoundOverlay(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SoundOverlay)
    , shown(false)
    , animationTimerOut(nullptr)
    , isAnimatingOut(false)
{
    ui->setupUi(this);
    setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint | Qt::WindowDoesNotAcceptFocus | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);

    timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, this, &SoundOverlay::animateOut);
    int playbackVolume = AudioManager::getPlaybackVolume();
    ui->progressBar->setValue(playbackVolume);
    ui->valueLabel->setPixmap(Utils::getIcon(1, AudioManager::getPlaybackVolume(), NULL).pixmap(16, 16));
}

SoundOverlay::~SoundOverlay()
{
    delete ui;
}

void SoundOverlay::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);  // Prevent unused parameter warning
    QPainter painter(this);

    // Get the main background color from the widget's palette
    QColor main_bg_color = this->palette().color(QPalette::Window);

    if (Utils::isDarkMode(main_bg_color)) {
        main_bg_color = main_bg_color.lighter(140);
    } else {
        main_bg_color = main_bg_color.darker(110);
    }
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
}

void SoundOverlay::animateIn()
{
    // Stop the out animation if it's running
    if (isAnimatingOut) {
        isAnimatingOut = false;
        if (animationTimerOut) {
            animationTimerOut->stop();
            animationTimerOut->deleteLater();
            animationTimerOut = nullptr;
        }
    }

    shown = true;
    QRect screenGeometry = QApplication::primaryScreen()->availableGeometry();
    int screenCenterX = screenGeometry.center().x();

    int panelX = screenCenterX - this->width() / 2;
    int startY = this->y(); // Start from the current position
    int targetY = screenGeometry.top() + this->height() - 12;

    this->show();

    const int durationMs = 300;
    const int refreshRate = 1;
    const double totalSteps = durationMs / refreshRate;

    int currentStep = 0;
    QTimer *animationTimerIn = new QTimer(this);

    animationTimerIn->start(refreshRate);

    connect(animationTimerIn, &QTimer::timeout, this, [=]() mutable {
        if (currentStep >= totalSteps) {
            animationTimerIn->stop();
            animationTimerIn->deleteLater();
            this->move(panelX, targetY);
            return;
        }

        double t = static_cast<double>(currentStep) / totalSteps;
        double easedT = 1 - pow(1 - t, 3);
        int currentY = startY + easedT * (targetY - startY);
        this->move(panelX, currentY);
        ++currentStep;
    });

    timer->start(2000);
}

void SoundOverlay::animateOut()
{
    if (isAnimatingOut)
        return;

    QRect screenGeometry = QApplication::primaryScreen()->geometry();
    int screenCenterX = screenGeometry.center().x();

    int panelX = screenCenterX - this->width() / 2;
    int startY = this->y();
    int targetY = screenGeometry.top() - this->height() - 12;

    const int durationMs = 300;
    const int refreshRate = 1;
    const double totalSteps = durationMs / refreshRate;

    int currentStep = 0;

    if (animationTimerOut)
        animationTimerOut->deleteLater();

    animationTimerOut = new QTimer(this);
    animationTimerOut->start(refreshRate);
    isAnimatingOut = true;

    connect(animationTimerOut, &QTimer::timeout, this, [=]() mutable {
        // If animateIn is called, stop the current animation
        if (!isAnimatingOut) {
            animationTimerOut->stop();
            animationTimerOut->deleteLater();
            animationTimerOut = nullptr;
            return;
        }

        if (currentStep >= totalSteps) {
            animationTimerOut->stop();
            animationTimerOut->deleteLater();
            animationTimerOut = nullptr;

            this->hide();
            isAnimatingOut = false;
            shown = false;
            return;
        }

        double t = static_cast<double>(currentStep) / totalSteps;
        double easedT = 1 - pow(1 - t, 3);
        int currentY = startY + easedT * (targetY - startY);
        this->move(panelX, currentY);
        ++currentStep;
    });
}

void SoundOverlay::toggleOverlay()
{
    if (!shown) {
        animateIn();
    } else {
        if (isAnimatingOut) {
            isAnimatingOut = false;
            if (animationTimerOut) {
                animationTimerOut->stop();
                animationTimerOut->deleteLater();
                animationTimerOut = nullptr;
            }
            animateIn();
        } else {
            timer->start(2000);
        }
    }
}

void SoundOverlay::updateVolumeIconAndLabel(QIcon icon, int volume)
{
    ui->iconLabel->setPixmap(icon.pixmap(16, 16));
    ui->valueLabel->setText(QString::number(volume));
    ui->progressBar->setValue(volume);
}

void SoundOverlay::updateMuteIcon(QIcon icon)
{
    ui->iconLabel->setPixmap(icon.pixmap(16, 16));
}

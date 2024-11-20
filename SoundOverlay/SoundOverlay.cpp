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
{
    ui->setupUi(this);
    setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint | Qt::WindowDoesNotAcceptFocus);
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

void SoundOverlay::animateIn()
{
    shown = true;
    QRect screenGeometry = QApplication::primaryScreen()->availableGeometry();
    int screenCenterX = screenGeometry.center().x();  // Get the center of the screen

    int panelX = screenCenterX - this->width() / 2; // Center the panel horizontally on the screen
    int startY = screenGeometry.top();  // Start from the top of the screen
    int targetY = screenGeometry.top() + this->height() - 12; // Final position (above tray icon)

    this->move(panelX, startY); // Start at the top
    this->show();

    // Animation parameters
    const int durationMs = 300;
    const int refreshRate = 1;
    const double totalSteps = durationMs / refreshRate;

    int currentStep = 0;
    QTimer *animationTimer = new QTimer(this);

    animationTimer->start(refreshRate);

    connect(animationTimer, &QTimer::timeout, this, [=]() mutable {
        if (currentStep >= totalSteps) {
            animationTimer->stop();
            animationTimer->deleteLater();
            this->move(panelX, targetY); // Ensure final position is set
            return;
        }

        double t = static_cast<double>(currentStep) / totalSteps; // Normalized time (0 to 1)
        // Easing function: Smooth deceleration
        double easedT = 1 - pow(1 - t, 3);

        // Interpolated Y position
        int currentY = startY + easedT * (targetY - startY);
        this->move(panelX, currentY);
        ++currentStep;
    });

    // Start the 2-second timer to animate out
    timer->start(2000);
}

void SoundOverlay::animateOut()
{
    QRect screenGeometry = QApplication::primaryScreen()->geometry();
    int screenCenterX = screenGeometry.center().x();  // Get the center of the screen

    int panelX = screenCenterX - this->width() / 2; // Center the panel horizontally on the screen
    int startY = this->y(); // Start from the current Y position
    int targetY = screenGeometry.top() - this->height() - 12; // Move to the top of the screen

    const int durationMs = 300;
    const int refreshRate = 1;
    const double totalSteps = durationMs / refreshRate;

    int currentStep = 0;
    QTimer *animationTimer = new QTimer(this);

    animationTimer->start(refreshRate);

    connect(animationTimer, &QTimer::timeout, this, [=]() mutable {
        if (currentStep >= totalSteps) {
            animationTimer->stop();
            animationTimer->deleteLater();

            this->hide();
            shown = false;
            return;
        }

        double t = static_cast<double>(currentStep) / totalSteps; // Normalized time (0 to 1)
        // Easing function: Smooth deceleration
        double easedT = 1 - pow(1 - t, 3);

        // Interpolated Y position
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
        // If already shown, reset the timer for 2 seconds
        timer->start(2000);
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

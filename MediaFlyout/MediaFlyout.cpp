#include "MediaFlyout.h"
#include "ui_MediaFlyout.h"
#include "Utils.h"
#include <QMessageBox>
#include <QDebug>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>
#include <QTimer>

MediaFlyout::MediaFlyout(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::MediaFlyout)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint | Qt::WindowDoesNotAcceptFocus | Qt::WindowStaysOnTopHint);
    this->setAttribute(Qt::WA_TranslucentBackground);
    setFixedWidth(width());
    borderColor = Utils::getTheme() == "light" ? QColor(255, 255, 255, 32) : QColor(0, 0, 0, 52);

    ui->prev->setIcon(Utils::getButtonsIcon("prev"));
    ui->next->setIcon(Utils::getButtonsIcon("next"));
    ui->pause->setIcon(Utils::getButtonsIcon("play"));
    updateIcon(Utils::getPlaceholderIcon());
    updateTitle("No song is currently playing.");
    updateArtist("");
    updateProgress(0, 0);
    ui->progressBar->setRange(0, 100);
    ui->pause->setEnabled(false);

    connect(ui->next, &QToolButton::clicked, this, &MediaFlyout::onNextClicked);
    connect(ui->prev, &QToolButton::clicked, this, &MediaFlyout::onPrevClicked);
    connect(ui->pause, &QToolButton::clicked, this, &MediaFlyout::onPauseClicked);
}

MediaFlyout::~MediaFlyout()
{
    delete ui;
}

void MediaFlyout::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);

    painter.setBrush(this->palette().color(QPalette::Window));
    painter.setPen(Qt::NoPen);

    QPainterPath path;
    path.addRoundedRect(this->rect().adjusted(1, 1, -1, -1), 8, 8);
    painter.drawPath(path);

    QPen borderPen(borderColor);
    borderPen.setWidth(1);
    painter.setPen(borderPen);
    painter.setBrush(Qt::NoBrush);

    QPainterPath borderPath;
    borderPath.addRoundedRect(this->rect().adjusted(0, 0, -1, -1), 8, 8);
    painter.drawPath(borderPath);
}

void MediaFlyout::animateIn()
{
    QRect screenGeometry = QApplication::primaryScreen()->geometry();
    int screenCenterX = screenGeometry.center().x();
    int margin = 12;
    int mediaFlyoutX = screenCenterX - this->width() / 2;
    int startY = screenGeometry.top() - this->height() - margin;
    int targetY = screenGeometry.top() + 12;

    this->move(mediaFlyoutX, startY);
    this->show();

    const int durationMs = 300;
    const int refreshRate = 1;
    const double totalSteps = durationMs / refreshRate;

    int currentStep = 0;
    QTimer *animationTimer = new QTimer(this);

    animationTimer->start(refreshRate);
    connect(animationTimer, &QTimer::timeout, this, [=]() mutable {
        double t = static_cast<double>(currentStep) / totalSteps;
        double easedT = 1 - pow(1 - t, 3);
        int currentY = startY + easedT * (targetY - startY);

        if (currentY == targetY) {
            animationTimer->stop();
            animationTimer->deleteLater();
            return;
        }

        this->move(mediaFlyoutX, currentY);
        ++currentStep;
    });
}

void MediaFlyout::animateOut(QRect trayIconGeometry)
{
    QRect screenGeometry = QApplication::primaryScreen()->geometry();
    int screenCenterX = screenGeometry.center().x();
    int panelX = screenCenterX - this->width() / 2;
    int startY = this->y();
    int targetY = screenGeometry.top() - this->height() - 12;

    const int durationMs = 300;
    const int refreshRate = 1;
    const double totalSteps = durationMs / refreshRate;

    int currentStep = 0;
    QTimer *animationTimer = new QTimer(this);

    animationTimer->start(refreshRate);
    connect(animationTimer, &QTimer::timeout, this, [=]() mutable {
        double t = static_cast<double>(currentStep) / totalSteps;
        double easedT = 1 - pow(1 - t, 3);
        int currentY = startY + easedT * (targetY - startY);

        if (currentY == targetY) {
            animationTimer->stop();
            animationTimer->deleteLater();
            return;
        }

        this->move(panelX, currentY);
        ++currentStep;
    });
}

QPixmap MediaFlyout::roundPixmap(const QPixmap &src, int radius) {
    if (src.isNull()) {
        return QPixmap();
    }

    QPixmap dest(src.size());
    dest.fill(Qt::transparent);

    QPainter painter(&dest);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    QPainterPath path;
    path.addRoundedRect(QRectF(0, 0, src.width(), src.height()), radius, radius);

    painter.setClipPath(path);
    painter.drawPixmap(0, 0, src);
    painter.end();

    return dest;
}

void MediaFlyout::onPrevClicked()
{
    emit requestPrev();
}

void MediaFlyout::onNextClicked()
{
    emit requestNext();
}

void MediaFlyout::onPauseClicked()
{
    emit requestPause();

}

void MediaFlyout::updateTitle(QString title)
{
    // Enable word wrap for the title QLabel
    ui->title->setWordWrap(true);

    // Limit the title to 2 lines
    QString fullTitle = title;
    QFontMetrics metrics(ui->title->font());
    int maxLines = 2;
    int lineHeight = metrics.lineSpacing(); // Height of a single line of text
    int maxHeight = maxLines * lineHeight;  // Maximum height for 2 lines

    // Calculate text that fits within the QLabel's width and max height
    QString truncatedTitle;
    QStringList words = fullTitle.split(' '); // Split title into words for manual wrapping
    QString currentLine;
    int currentHeight = 0;

    for (const QString& word : words) {
        // Test appending the word to the current line
        QString testLine = currentLine.isEmpty() ? word : currentLine + ' ' + word;

        // Check if the current line exceeds the QLabel's width
        if (metrics.horizontalAdvance(testLine) > ui->title->width()) {
            // Line is too wide, add it as a completed line
            if (!truncatedTitle.isEmpty()) {
                truncatedTitle += '\n';
            }
            truncatedTitle += currentLine;
            currentHeight += lineHeight;

            // Start a new line with the current word
            currentLine = word;

            // Stop if we've reached the maximum number of lines
            if (currentHeight >= maxHeight) {
                truncatedTitle.chop(3); // Remove last characters if needed
                truncatedTitle += "..."; // Add ellipsis
                break;
            }
        } else {
            // Line is still within width, keep building it
            currentLine = testLine;
        }
    }

    // Add the last line, if there's space left
    if (currentHeight < maxHeight && !currentLine.isEmpty()) {
        if (!truncatedTitle.isEmpty()) {
            truncatedTitle += '\n';
        }
        truncatedTitle += currentLine;
    }

    // Set the processed title text
    ui->title->setText(truncatedTitle);
}

void MediaFlyout::updateArtist(QString artist)
{
    ui->artist->setText(artist);
}

void MediaFlyout::updateIcon(QIcon icon)
{
    QPixmap originalIcon = icon.pixmap(96, 96);
    QPixmap roundedIcon = roundPixmap(originalIcon, 8);
    ui->icon->setPixmap(roundedIcon);
}

void MediaFlyout::updatePauseButton(QString playbackState)
{
    ui->pause->setEnabled(true);
    QString playPause;
    if (playbackState == "Playing") {
        playPause = "pause";
    } else {
        playPause = "play";
    }
    ui->pause->setIcon(Utils::getButtonsIcon(playPause));
}

void MediaFlyout::updateControls(bool prev, bool next)
{
    ui->next->setEnabled(next);
    ui->prev->setEnabled(prev);
}

void MediaFlyout::updateProgress(int current, int total)
{
    ui->progressBar->setRange(0, total);
    ui->progressBar->setValue(current);

    int currentMinutes = current / 60;
    int currentSeconds = current % 60;
    QString currentTimeText = QString::asprintf("%02d:%02d", currentMinutes, currentSeconds);

    int totalMinutes = total / 60;
    int totalSeconds = total % 60;
    QString totalTimeText = QString::asprintf("%02d:%02d", totalMinutes, totalSeconds);

    ui->currentTime->setText(currentTimeText);
    ui->totalTime->setText(totalTimeText);
}


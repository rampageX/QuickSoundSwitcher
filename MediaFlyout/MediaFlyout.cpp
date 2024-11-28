#include "MediaFlyout.h"
#include "ui_MediaFlyout.h"
#include "Utils.h"
#include <QMessageBox>
#include <QDebug>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>
#include <QTimer>
#include <QPropertyAnimation>

MediaFlyout::MediaFlyout(QWidget* parent, MediaSessionWorker *worker)
    : QWidget(parent)
    , ui(new Ui::MediaFlyout)
    , worker(worker)
    , currentlyPlaying(false)
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
    updateTitle(tr("No song is currently playing."));
    updateArtist("");
    updateProgress(0, 0);
    ui->progressBar->setRange(0, 100);
    ui->pause->setEnabled(false);

    connect(ui->next, &QToolButton::clicked, this, &MediaFlyout::onNextClicked);
    connect(ui->prev, &QToolButton::clicked, this, &MediaFlyout::onPrevClicked);
    connect(ui->pause, &QToolButton::clicked, this, &MediaFlyout::onPauseClicked);

    connect(worker, &MediaSessionWorker::titleChanged, this, &MediaFlyout::updateTitle);
    connect(worker, &MediaSessionWorker::artistChanged, this, &MediaFlyout::updateArtist);
    connect(worker, &MediaSessionWorker::sessionIconChanged, this, &MediaFlyout::updateIcon);
    connect(worker, &MediaSessionWorker::sessionPlaybackStateChanged, this, &MediaFlyout::updatePauseButton);
    connect(worker, &MediaSessionWorker::sessionTimeInfoUpdated, this, &MediaFlyout::updateProgress);

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
    int targetY = screenGeometry.top() + margin;

    this->move(mediaFlyoutX, targetY);

    QPropertyAnimation *animation = new QPropertyAnimation(this, "windowOpacity");
    animation->setDuration(300);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    animation->setEasingCurve(QEasingCurve::Linear);

    this->setWindowOpacity(0.0);
    this->show();

    QObject::connect(animation, &QPropertyAnimation::finished, [=]() {
        animation->deleteLater();
    });

    animation->start();
}

void MediaFlyout::animateOut()
{
    QPropertyAnimation *animation = new QPropertyAnimation(this, "windowOpacity");
    animation->setDuration(300);
    animation->setStartValue(1.0);
    animation->setEndValue(0.0);
    animation->setEasingCurve(QEasingCurve::Linear);

    QObject::connect(animation, &QPropertyAnimation::finished, this, [=, this]() {
        animation->deleteLater();
        emit mediaFlyoutClosed();
    });

    animation->start();
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
    worker->previous();
}

void MediaFlyout::onNextClicked()
{
    worker->next();
}

void MediaFlyout::onPauseClicked()
{
    if (!currentlyPlaying) {
        worker->play();
    } else {
        worker->pause();
    }
}

void MediaFlyout::updateTitle(QString title)
{
    QFontMetrics metrics(ui->title->font());
    ui->title->setWordWrap(true);
    ui->title->setText(Utils::truncateTitle(title, metrics, ui->title->width()));
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

void MediaFlyout::updatePauseButton(QString state)
{
    ui->pause->setEnabled(true);
    QString playPause;
    if (state == "Playing") {
        playPause = "pause";
        currentlyPlaying = true;
    } else {
        playPause = "play";
        currentlyPlaying = false;
    }
    ui->pause->setIcon(Utils::getButtonsIcon(playPause));
}

void MediaFlyout::updateControls(bool prev, bool next)
{
    ui->next->setEnabled(next);
    ui->prev->setEnabled(prev);
}

void MediaFlyout::updateProgress(int currentTime, int totalTime)
{
    int currentMinutes = currentTime / 60;
    int currentSeconds = currentTime % 60;
    QString currentTimeText = QString::asprintf("%02d:%02d", currentMinutes, currentSeconds);

    int totalMinutes = totalTime / 60;
    int totalSeconds = totalTime % 60;
    QString totalTimeText = QString::asprintf("%02d:%02d", totalMinutes, totalSeconds);

    ui->currentTime->setText(currentTimeText);
    ui->totalTime->setText(totalTimeText);
    ui->progressBar->setRange(0, totalTime);
    ui->progressBar->setValue(currentTime);
}


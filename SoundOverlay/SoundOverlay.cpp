#include "SoundOverlay.h"
#include "Utils.h"
#include <QTimer>
#include <QApplication>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>
#include <QPropertyAnimation>
#include <QQmlContext>
#include <QWindow>
#include <QBuffer>
#include <QImageWriter>
#include <QByteArray>
#include <QPixmap>

SoundOverlay::SoundOverlay(QObject *parent)
    : QObject(parent)
    , shown(false)
    , animationTimerOut(nullptr)
    , isAnimatingOut(false)
{

    engine = new QQmlApplicationEngine(this);
    engine->rootContext()->setContextProperty("soundOverlay", this);
    engine->rootContext()->setContextProperty("volumeIconPixmap", ""); // Initial value
    engine->rootContext()->setContextProperty("volumeLabelText", "50");
    engine->rootContext()->setContextProperty("volumeBarValue", 50);
    engine->load(QUrl(QStringLiteral("qrc:/qml/SoundOverlay.qml")));

    expireTimer = new QTimer(this);
    connect(expireTimer, &QTimer::timeout, this, &SoundOverlay::animateOut);
}

SoundOverlay::~SoundOverlay()
{
}

void SoundOverlay::applyRadius(QWindow *window, int radius)
{
    QRect r(QPoint(), window->geometry().size());
    QRect rb(0, 0, 2 * radius, 2 * radius);

    QRegion region(rb, QRegion::Ellipse);
    rb.moveRight(r.right());
    region += QRegion(rb, QRegion::Ellipse);
    rb.moveBottom(r.bottom());
    region += QRegion(rb, QRegion::Ellipse);
    rb.moveLeft(r.left());
    region += QRegion(rb, QRegion::Ellipse);
    region += QRegion(r.adjusted(radius, 0, -radius, 0), QRegion::Rectangle);
    region += QRegion(r.adjusted(0, radius, 0, -radius), QRegion::Rectangle);
    window->setMask(region);
}


void SoundOverlay::animateIn()
{
    if (isAnimatingOut) return;

    if (shown) {
        expireTimer->start(3000);
        return;
    }

    shown = true;

    QWindow *qmlWindow = qobject_cast<QWindow*>(engine->rootObjects().first());
    if (!qmlWindow) return;

    applyRadius(qmlWindow, 8);

    QRect screenGeometry = QApplication::primaryScreen()->availableGeometry();
    int screenCenterX = screenGeometry.center().x();
    int margin = 12;
    int windowWidth = qmlWindow->width();
    int windowHeight = qmlWindow->height();

    int soundOverlayX = screenCenterX - windowWidth / 2;
    int startY = screenGeometry.top() - windowHeight;
    int targetY = screenGeometry.top() + margin;

    qmlWindow->setPosition(soundOverlayX, startY);
    qmlWindow->setVisible(true);

    const int durationMs = 200;
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
            expireTimer->start(3000);
            return;
        }

        qmlWindow->setPosition(soundOverlayX, currentY);
        ++currentStep;
    });
}

void SoundOverlay::animateOut()
{
    shown = false;
    isAnimatingOut = true;

    QWindow *qmlWindow = qobject_cast<QWindow*>(engine->rootObjects().first());
    if (!qmlWindow) return;

    QRect screenGeometry = QApplication::primaryScreen()->availableGeometry();
    int windowHeight = qmlWindow->height();
    int startY = qmlWindow->y();
    int targetY = screenGeometry.top() - windowHeight;
    int soundOverlayX = qmlWindow->x();

    const int durationMs = 200;
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
            qmlWindow->setVisible(false);
            isAnimatingOut = false;
            return;
        }

        qmlWindow->setPosition(soundOverlayX, currentY);
        ++currentStep;
    });
}

void SoundOverlay::showOverlay()
{
    if (!engine->rootObjects().isEmpty()) {
        QObject *rootObject = engine->rootObjects().first();
        rootObject->setProperty("visible", true);
    }
}

void SoundOverlay::updateVolumeIcon(const QString &icon)
{
    engine->rootContext()->setContextProperty("volumeIconPixmap", QStringLiteral("qrc:/icons/") + icon);
}

void SoundOverlay::updateVolumeLabel(int volume)
{
    engine->rootContext()->setContextProperty("volumeLabelText", QString::number(volume));
}

void SoundOverlay::updateProgressBar(int volume)
{
    engine->rootContext()->setContextProperty("volumeBarValue", volume);
}

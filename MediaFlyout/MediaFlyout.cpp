#include "MediaFlyout.h"
#include "MediaSessionWorker.h"
#include "Utils.h"
#include <QQmlContext>
#include <QQuickItem>
#include <QIcon>
#include <QPixmap>
#include <QBuffer>

MediaFlyout::MediaFlyout(QObject* parent, MediaSessionWorker* worker)
    : QObject(parent)
    , worker(worker)
    , m_currentTime(0)
    , m_totalTime(100)
    , m_isPlaying(false)
    , m_isPrevEnabled(true)  // default enabled state
    , m_isNextEnabled(true)  // default enabled state
{
    engine = new QQmlApplicationEngine(this);
    engine->rootContext()->setContextProperty("mediaFlyout", this);

    QColor windowColor = QGuiApplication::palette().color(QPalette::Window);
    // Expose the native window color to QML
    engine->rootContext()->setContextProperty("nativeWindowColor", windowColor);


    engine->load(QUrl(QStringLiteral("qrc:/qml/MediaFlyout.qml")));

    mediaFlyoutWindow = qobject_cast<QWindow*>(engine->rootObjects().first());


    connect(worker, &MediaSessionWorker::titleChanged, this, &MediaFlyout::updateTitle);
    connect(worker, &MediaSessionWorker::artistChanged, this, &MediaFlyout::updateArtist);
    connect(worker, &MediaSessionWorker::sessionIconChanged, this, &MediaFlyout::updateIcon);
    connect(worker, &MediaSessionWorker::sessionPlaybackStateChanged, this, &MediaFlyout::updatePauseButton);
    connect(worker, &MediaSessionWorker::sessionTimeInfoUpdated, this, &MediaFlyout::updateProgress);

    setDefaults();
}

MediaFlyout::~MediaFlyout()
{
    delete engine;
}

QString MediaFlyout::title() const { return m_title; }
void MediaFlyout::setTitle(const QString& title) {
    if (m_title != title) {
        m_title = title;
        emit titleChanged(m_title);
    }
}

QString MediaFlyout::artist() const { return m_artist; }
void MediaFlyout::setArtist(const QString& artist) {
    if (m_artist != artist) {
        m_artist = artist;
        emit artistChanged(m_artist);
    }
}

int MediaFlyout::currentTime() const { return m_currentTime; }
void MediaFlyout::setCurrentTime(int currentTime) {
    if (m_currentTime != currentTime) {
        m_currentTime = currentTime;
        emit currentTimeChanged(m_currentTime);
    }
}

int MediaFlyout::totalTime() const { return m_totalTime; }
void MediaFlyout::setTotalTime(int totalTime) {
    if (m_totalTime != totalTime) {
        m_totalTime = totalTime;
        emit totalTimeChanged(m_totalTime);
    }
}

bool MediaFlyout::isPlaying() const { return m_isPlaying; }
void MediaFlyout::setIsPlaying(bool isPlaying) {
    if (m_isPlaying != isPlaying) {
        m_isPlaying = isPlaying;
        emit isPlayingChanged(m_isPlaying);
    }
}

QString MediaFlyout::iconSource() const { return m_iconSource; }
void MediaFlyout::setIconSource(const QString& iconSource) {
    if (m_iconSource != iconSource) {
        m_iconSource = iconSource;
        emit iconSourceChanged(m_iconSource);
    }
}

bool MediaFlyout::isPrevEnabled() const { return m_isPrevEnabled; }
void MediaFlyout::setPrevEnabled(bool enabled) {
    if (m_isPrevEnabled != enabled) {
        m_isPrevEnabled = enabled;
        emit prevEnabledChanged(m_isPrevEnabled);
    }
}

bool MediaFlyout::isNextEnabled() const { return m_isNextEnabled; }
void MediaFlyout::setNextEnabled(bool enabled) {
    if (m_isNextEnabled != enabled) {
        m_isNextEnabled = enabled;
        emit nextEnabledChanged(m_isNextEnabled);
    }
}

void MediaFlyout::applyRadius(QWindow *window, int radius)
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

void MediaFlyout::updateTitle(QString title)
{
    if (title.length() > 30) {
        title = title.left(30) + "...";
    }
    setTitle(title);
}

void MediaFlyout::updateArtist(QString artist)
{
    if (artist.length() > 30) {
        artist = artist.left(30) + "...";
    }
    setArtist(artist);
}

void MediaFlyout::updateIcon(QIcon icon) {
    QPixmap pixmap = icon.pixmap(64, 64);
    pixmap = Utils::roundPixmap(pixmap, 8);
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    pixmap.save(&buffer, "PNG");
    setIconSource("data:image/png;base64," + byteArray.toBase64());
}

void MediaFlyout::updatePauseButton(QString state) {
    bool isPlaying = (state == "Playing");
    setIsPlaying(isPlaying);
}

void MediaFlyout::updateProgress(int currentTime, int totalTime) {
    setCurrentTime(currentTime);
    setTotalTime(totalTime);
}

void MediaFlyout::setDefaults() {
    setTitle("No song is currently playing");
    setArtist("");
    setCurrentTime(0);
    setTotalTime(100);
    setIsPlaying(false);
    setIconSource("qrc:/icons/placeholder_light.png");
}

void MediaFlyout::onPrevButtonClicked() {
    worker->previous();
}

void MediaFlyout::onPauseButtonClicked() {
    if (isPlaying()) {
        worker->pause();
    } else {
        worker->play();
    }
}

void MediaFlyout::onNextButtonClicked() {
    worker->next();
}

void MediaFlyout::updateControls(bool prev, bool next) {
    setPrevEnabled(prev);
    setNextEnabled(next);
}

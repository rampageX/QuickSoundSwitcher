#include "mediasessionmanager.h"
#include <QMetaObject>
#include <QMutexLocker>
#include <QDebug>
#include <QBuffer>
#include <QByteArray>
#include <QPixmap>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <windows.h>
#include <winrt/Windows.Media.Control.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.Streams.h>

using namespace winrt;
using namespace Windows::Media::Control;
using namespace Windows::Foundation;
using namespace Windows::Storage::Streams;

static QThread* g_mediaWorkerThread = nullptr;
static MediaWorker* g_mediaWorker = nullptr;
static QMutex g_mediaInitMutex;

QPixmap createRoundedPixmap(const QPixmap& source, int targetSize, int radius) {
    // Scale the source to target size while maintaining aspect ratio
    QPixmap scaled = source.scaled(targetSize, targetSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);

    // Create a new pixmap with transparent background
    QPixmap rounded(targetSize, targetSize);
    rounded.fill(Qt::transparent);

    QPainter painter(&rounded);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    // Create a rounded rectangle path
    QPainterPath path;
    path.addRoundedRect(0, 0, targetSize, targetSize, radius, radius);

    // Set the clipping path
    painter.setClipPath(path);

    // Calculate position to center the scaled image
    int x = (targetSize - scaled.width()) / 2;
    int y = (targetSize - scaled.height()) / 2;

    // Draw the scaled image
    painter.drawPixmap(x, y, scaled);

    return rounded;
}

MediaInfo queryMediaInfoImpl() {
    MediaInfo info;

    try {
        init_apartment();
        auto sessionManager = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
        auto currentSession = sessionManager.GetCurrentSession();

        if (currentSession) {
            auto properties = currentSession.TryGetMediaPropertiesAsync().get();
            if (properties) {
                info.title = QString::fromWCharArray(properties.Title().c_str());
                info.artist = QString::fromWCharArray(properties.Artist().c_str());
                info.album = QString::fromWCharArray(properties.AlbumTitle().c_str());

                // Fetch album art
                try {
                    auto thumbnailRef = properties.Thumbnail();
                    if (thumbnailRef) {
                        auto thumbnailStream = thumbnailRef.OpenReadAsync().get();
                        if (thumbnailStream) {
                            auto size = thumbnailStream.Size();
                            if (size > 0) {
                                DataReader reader(thumbnailStream);
                                auto bytesLoaded = reader.LoadAsync(static_cast<uint32_t>(size)).get();

                                if (bytesLoaded > 0) {
                                    std::vector<uint8_t> buffer(bytesLoaded);
                                    reader.ReadBytes(buffer);

                                    QByteArray originalImageData(reinterpret_cast<const char*>(buffer.data()), bytesLoaded);

                                    // Load the image and create rounded corners
                                    QPixmap originalPixmap;
                                    if (originalPixmap.loadFromData(originalImageData)) {
                                        // Create a rounded version
                                        int targetSize = 64; // Target size for the rounded image
                                        QPixmap roundedPixmap = createRoundedPixmap(originalPixmap, targetSize, 8);

                                        // Convert back to base64
                                        QByteArray processedImageData;
                                        QBuffer buffer(&processedImageData);
                                        buffer.open(QIODevice::WriteOnly);
                                        roundedPixmap.save(&buffer, "PNG");

                                        QString base64Image = processedImageData.toBase64();
                                        info.albumArt = QString("data:image/png;base64,%1").arg(base64Image);
                                    }
                                }
                            }
                        }
                    }
                } catch (...) {
                    qDebug() << "Failed to fetch album art";
                }
            }

            auto playbackInfo = currentSession.GetPlaybackInfo();
            if (playbackInfo) {
                info.isPlaying = (playbackInfo.PlaybackStatus() == GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing);
            }
        }
    } catch (...) {
        qDebug() << "Failed to query media session info";
    }

    return info;
}

void MediaWorker::initializeTimer() {
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &MediaWorker::queryMediaInfo);
}

void MediaWorker::queryMediaInfo() {
    MediaInfo info = queryMediaInfoImpl();
    emit mediaInfoChanged(info);
}

void MediaWorker::startMonitoring() {
    if (!m_updateTimer) {
        initializeTimer();
    }
    m_updateTimer->start(2000); // Update every 2 seconds
}

void MediaWorker::stopMonitoring() {
    if (m_updateTimer) {
        m_updateTimer->stop();
    }
}

void MediaSessionManager::initialize() {
    QMutexLocker locker(&g_mediaInitMutex);

    if (!g_mediaWorkerThread) {
        g_mediaWorkerThread = new QThread;
        g_mediaWorker = new MediaWorker;
        g_mediaWorker->moveToThread(g_mediaWorkerThread);
        g_mediaWorkerThread->start();
    }
}

void MediaSessionManager::cleanup() {
    QMutexLocker locker(&g_mediaInitMutex);

    if (g_mediaWorkerThread) {
        g_mediaWorkerThread->quit();
        g_mediaWorkerThread->wait();
        delete g_mediaWorker;
        delete g_mediaWorkerThread;
        g_mediaWorker = nullptr;
        g_mediaWorkerThread = nullptr;
    }
}

void MediaSessionManager::queryMediaInfoAsync() {
    if (g_mediaWorker) {
        QMetaObject::invokeMethod(g_mediaWorker, "queryMediaInfo", Qt::QueuedConnection);
    }
}

void MediaSessionManager::startMonitoringAsync() {
    if (g_mediaWorker) {
        QMetaObject::invokeMethod(g_mediaWorker, "startMonitoring", Qt::QueuedConnection);
    }
}

void MediaSessionManager::stopMonitoringAsync() {
    if (g_mediaWorker) {
        QMetaObject::invokeMethod(g_mediaWorker, "stopMonitoring", Qt::QueuedConnection);
    }
}

MediaWorker* MediaSessionManager::getWorker() {
    return g_mediaWorker;
}

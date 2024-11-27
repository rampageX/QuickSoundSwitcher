#ifndef MEDIASESSIONWORKER_H
#define MEDIASESSIONWORKER_H

#include <QObject>
#include "MediaSession.h"
#include <windows.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.Control.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.System.h>

class MediaSessionWorker : public QObject
{
    Q_OBJECT

public:
    explicit MediaSessionWorker(QObject* parent = nullptr);
    ~MediaSessionWorker();
    void process();
    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void next();
    Q_INVOKABLE void previous();
    void initializeSessionMonitoring();

private:
    winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSession currentSession;
    bool monitoringEnabled;

signals:
    void sessionReady(const MediaSession& session);
    void sessionError();

    void sessionMediaPropertiesChanged(const QString& title, const QString& artist);
    void sessionPlaybackStateChanged(const QString& state);
    void sessionIconChanged(QIcon icon);
    void sessionTimeInfoUpdated(int currentTime, int totalTime);
};

#endif // MEDIASESSIONWORKER_H

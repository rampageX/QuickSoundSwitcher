#ifndef MEDIASFLYOUT_H
#define MEDIASFLYOUT_H

#include <QObject>
#include <QQmlApplicationEngine>
#include "MediaSessionWorker.h"
#include <QWindow>
#include <QIcon>

class MediaFlyout : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(QString artist READ artist WRITE setArtist NOTIFY artistChanged)
    Q_PROPERTY(int currentTime READ currentTime WRITE setCurrentTime NOTIFY currentTimeChanged)
    Q_PROPERTY(int totalTime READ totalTime WRITE setTotalTime NOTIFY totalTimeChanged)
    Q_PROPERTY(bool isPlaying READ isPlaying WRITE setIsPlaying NOTIFY isPlayingChanged)
    Q_PROPERTY(QString iconSource READ iconSource WRITE setIconSource NOTIFY iconSourceChanged)
    Q_PROPERTY(bool isPrevEnabled READ isPrevEnabled WRITE setPrevEnabled NOTIFY prevEnabledChanged)
    Q_PROPERTY(bool isNextEnabled READ isNextEnabled WRITE setNextEnabled NOTIFY nextEnabledChanged)

public:
    explicit MediaFlyout(QObject* parent = nullptr, MediaSessionWorker* worker = nullptr);
    ~MediaFlyout() override;

    QWindow* mediaFlyoutWindow;

    QString title() const;
    void setTitle(const QString& title);

    QString artist() const;
    void setArtist(const QString& artist);

    int currentTime() const;
    void setCurrentTime(int currentTime);

    int totalTime() const;
    void setTotalTime(int totalTime);

    bool isPlaying() const;
    void setIsPlaying(bool isPlaying);

    QString iconSource() const;
    void setIconSource(const QString& iconSource);

    bool isPrevEnabled() const;
    void setPrevEnabled(bool enabled);

    bool isNextEnabled() const;
    void setNextEnabled(bool enabled);

    void setDefaults();
    void applyRadius(QWindow *window, int radius);

public slots:
    void onPrevButtonClicked();
    void onPauseButtonClicked();
    void onNextButtonClicked();
    void updateTitle(QString title);
    void updateArtist(QString artist);
    void updateIcon(QIcon icon);
    void updatePauseButton(QString state);
    void updateProgress(int currentTime, int totalTime);
    void updateControls(bool prev, bool next);

signals:
    void titleChanged(QString);
    void artistChanged(QString);
    void currentTimeChanged(int);
    void totalTimeChanged(int);
    void isPlayingChanged(bool);
    void iconSourceChanged(QString);
    void prevEnabledChanged(bool);
    void nextEnabledChanged(bool);
    void mediaFlyoutClosed();

private:
    QQmlApplicationEngine* engine;
    MediaSessionWorker* worker;

    QString m_title;
    QString m_artist;
    int m_currentTime;
    int m_totalTime;
    bool m_isPlaying;
    QString m_iconSource;
    bool m_isPrevEnabled;
    bool m_isNextEnabled;
};

#endif // MEDIASFLYOUT_H

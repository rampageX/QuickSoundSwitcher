#ifndef SOUNDOVERLAY_H
#define SOUNDOVERLAY_H

#include <QObject>
#include <QQmlApplicationEngine>
#include <QWindow>

class SoundOverlay : public QObject
{
    Q_OBJECT

public:
    explicit SoundOverlay(QObject *parent = nullptr);
    ~SoundOverlay();
    void animateIn();

    void updateVolumeIcon(const QString &icon);
    void updateVolumeLabel(int volume);
    void updateProgressBar(int volume);
    void showOverlay();

private:
    bool shown;
    QTimer *expireTimer;
    QTimer *raiseTimer;
    void animateOut();
    QTimer *animationTimerOut;
    bool isAnimatingOut;
    void applyRadius(QWindow *window, int radius);

    QQmlApplicationEngine *engine;

signals:
    void overlayClosed();
};

#endif // SOUNDOVERLAY_H

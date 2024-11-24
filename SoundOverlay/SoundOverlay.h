#ifndef SOUNDOVERLAY_H
#define SOUNDOVERLAY_H

#include <QWidget>

namespace Ui {
class SoundOverlay;
}

class SoundOverlay : public QWidget
{
    Q_OBJECT

public:
    explicit SoundOverlay(QWidget *parent = nullptr);
    ~SoundOverlay();
    void toggleOverlay(int mediaFlyoutHeight);
    void updateVolumeIconAndLabel(QIcon icon, int volume);
    void updateMuteIcon(QIcon icon);
    void moveToPosition(int mediaFlyoutHeight);
    bool movedToPosition;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    Ui::SoundOverlay *ui;
    bool shown;
    QTimer *timer;
    QTimer *raiseTimer;
    void animateIn(int mediaFlyoutHeight);
    void animateOut();
    QTimer *animationTimerOut;
    bool isAnimatingOut;

    int remainingTime;
    void pauseExpireTimer();
    void resumeExpireTimer();

signals:
    void overlayClosed();
};

#endif // SOUNDOVERLAY_H

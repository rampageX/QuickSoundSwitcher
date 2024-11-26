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
    void animateIn();
    void updateVolumeIconAndLabel(QIcon icon, int volume);
    void updateMuteIcon(QIcon icon);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    Ui::SoundOverlay *ui;
    bool shown;
    QTimer *expireTimer;
    QTimer *raiseTimer;
    void animateOut();
    QTimer *animationTimerOut;
    bool isAnimatingOut;

signals:
    void overlayClosed();
};

#endif // SOUNDOVERLAY_H

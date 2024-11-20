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
    void toggleOverlay();
    void updateVolumeIconAndLabel(QIcon icon, int volume);
    void updateMuteIcon(QIcon icon);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    Ui::SoundOverlay *ui;
    bool shown;
    QTimer *timer;
    QTimer *raiseTimer;
    void animateIn();
    void animateOut();
};

#endif // SOUNDOVERLAY_H

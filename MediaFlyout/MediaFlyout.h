#ifndef MEDIASFLYOUT_H
#define MEDIASFLYOUT_H

#include <QWidget>
#include <QThread>

namespace Ui {
class MediaFlyout;
}

class MediaFlyout : public QWidget
{
    Q_OBJECT

public:
    explicit MediaFlyout(QWidget* parent = nullptr);
    ~MediaFlyout() override;
    void animateIn();
    void animateOut(QRect trayIconGeometry);
    void updateTitle(QString title);
    void updateArtist(QString artist);
    void updateIcon(QIcon icon);
    void updatePauseButton(QString playbackState);
    void updateControls(bool prev, bool next);
    void updateProgress(int current, int total);

public slots:
    void onPrevClicked();
    void onNextClicked();
    void onPauseClicked();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    Ui::MediaFlyout *ui;
    QColor borderColor;
    QPixmap roundPixmap(const QPixmap &src, int radius);
    int currentTime;
    int totalTime;
    bool isAnimating;

signals:
    void requestPrev();
    void requestNext();
    void requestPause();
};

#endif // MEDIASFLYOUT_H

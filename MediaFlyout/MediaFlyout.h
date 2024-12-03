#ifndef MEDIASFLYOUT_H
#define MEDIASFLYOUT_H

#include <QWidget>
#include <QThread>
#include "MediaSessionWorker.h"

namespace Ui {
class MediaFlyout;
}

class MediaFlyout : public QWidget
{
    Q_OBJECT

public:
    explicit MediaFlyout(QWidget* parent = nullptr, MediaSessionWorker *worker = nullptr);
    ~MediaFlyout() override;
    void updateTitle(QString title);
    void updateArtist(QString artist);
    void updateIcon(QIcon icon);
    void updatePauseButton(QString state);
    void updateControls(bool prev, bool next);
    void updateProgress(int currentTime, int totalTime);
    void setDefaults();

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
    MediaSessionWorker *worker;
    bool currentlyPlaying;

signals:
    void mediaFlyoutClosed();
};

#endif // MEDIASFLYOUT_H

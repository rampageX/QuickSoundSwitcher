#ifndef OVERLAYWIDGET_H
#define OVERLAYWIDGET_H

#include <QWidget>
#include <QTimer>

class OverlayWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OverlayWidget(QString& position, bool& potatoMode, QWidget *parent = nullptr);
    void moveOverlayToPosition(const QString &position);

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void updateOpacity();

private:
    QPixmap cachedOverlayIcon;
    QTimer *timer;
    double opacityFactor;
    bool increasing;
};

#endif // OVERLAYWIDGET_H

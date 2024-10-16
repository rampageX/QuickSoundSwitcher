#ifndef CUSTOMTRAYICON_H
#define CUSTOMTRAYICON_H

#include <QSystemTrayIcon>
#include <QEvent>
#include <QWheelEvent>

class CustomTrayIcon : public QSystemTrayIcon {
    Q_OBJECT

public:
    explicit CustomTrayIcon(QObject *parent = nullptr) : QSystemTrayIcon(parent) {}

signals:
    void wheelScrolled(int delta);

protected:
    bool event(QEvent *event) override {
        if (event->type() == QEvent::Wheel) {
            QWheelEvent *wheelEvent = static_cast<QWheelEvent *>(event);
            emit wheelScrolled(wheelEvent->angleDelta().y());
            return true;
        }
        return QSystemTrayIcon::event(event);
    }
};

#endif // CUSTOMTRAYICON_H

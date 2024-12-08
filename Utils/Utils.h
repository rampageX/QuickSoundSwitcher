#ifndef UTILS_H
#define UTILS_H

#include <QIcon>
#include <QFrame>

namespace Utils {
    QString getIcon(int type, int volume, bool muted);
    void setFrameColorBasedOnWindow(QWidget *window, QFrame *frame);
    QColor adjustColor(const QColor &color, double factor);
    bool isDarkMode(const QColor &color);
    QIcon generateMutedIcon(QPixmap originalPixmap);
    QString getAccentColor(const QString &accentKey);
    QString getTheme();
    QIcon getPlaceholderIcon();
    QIcon getButtonsIcon(QString button);
    void setAlwaysOnTopState(QWidget *widget, bool state);
    QPixmap roundPixmap(const QPixmap &src, int radius);
    QString getOverlayIcon(int volume);
    void playSoundNotification();
}

#endif // UTILS_H

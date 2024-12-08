#ifndef UTILS_H
#define UTILS_H

#include <QIcon>
#include <QFrame>

namespace Utils {
    QString getIcon(int type, int volume, bool muted);
    QString getAccentColor(const QString &accentKey);
    QString getTheme();
    void playSoundNotification();
}

#endif // UTILS_H

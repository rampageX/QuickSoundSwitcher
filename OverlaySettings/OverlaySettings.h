#ifndef OVERLAYSETTINGS_H
#define OVERLAYSETTINGS_H

#include <QMainWindow>
#include <QSettings>

namespace Ui {
class OverlaySettings;
}

class OverlaySettings : public QMainWindow
{
    Q_OBJECT

public:
    explicit OverlaySettings(QWidget *parent = nullptr);
    ~OverlaySettings();

private slots:
    void onDisableOverlayStateChanged(Qt::CheckState state);

private:
    Ui::OverlaySettings *ui;
    QSettings settings;
    void loadSettings();
    void saveSettings();

signals:
    void settingsChanged();
    void closed();
};

#endif // OVERLAYSETTINGS_H

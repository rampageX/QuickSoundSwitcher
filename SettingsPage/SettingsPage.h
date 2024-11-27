#ifndef SETTINGSPAGE_H
#define SETTINGSPAGE_H

#include <QMainWindow>
#include <QSettings>

namespace Ui {
class SettingsPage;
}

class SettingsPage : public QMainWindow
{
    Q_OBJECT

public:
    explicit SettingsPage(QWidget *parent = nullptr);
    ~SettingsPage();

private:
    Ui::SettingsPage *ui;
    QSettings settings;
    void loadSettings();
    void saveSettings();

signals:
    void settingsChanged();
    void closed();
};

#endif // SETTINGSPAGE_H

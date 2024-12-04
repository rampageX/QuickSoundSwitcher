#ifndef SETTINGSPAGE_H
#define SETTINGSPAGE_H

#include <QObject>
#include <QSettings>
#include <QQmlApplicationEngine>
#include <QQmlContext>

class SettingsPage : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int volumeIncrement READ volumeIncrement WRITE setVolumeIncrement NOTIFY settingsChanged)
    Q_PROPERTY(bool mergeSimilarApps READ mergeSimilarApps WRITE setMergeSimilarApps NOTIFY settingsChanged)

public:
    explicit SettingsPage(QObject *parent = nullptr);
    ~SettingsPage();

    int volumeIncrement() const;
    void setVolumeIncrement(int value);

    bool mergeSimilarApps() const;
    void setMergeSimilarApps(bool value);

    void showWindow();

signals:
    void settingsChanged();

private:
    QSettings settings;
    QQmlApplicationEngine *engine = nullptr;
};

#endif // SETTINGSPAGE_H

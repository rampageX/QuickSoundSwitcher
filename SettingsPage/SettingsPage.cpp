#include "SettingsPage.h"
#include <QQmlApplicationEngine>
#include <QQmlContext>

SettingsPage::SettingsPage(QObject *parent)
    : QObject(parent), settings("Odizinne", "QuickSoundSwitcher")
{
    engine = new QQmlApplicationEngine(this);

    engine->rootContext()->setContextProperty("settingsPage", this);
    engine->load(QUrl(QStringLiteral("qrc:/qml/SettingsPage.qml")));
}

SettingsPage::~SettingsPage()
{
}

int SettingsPage::volumeIncrement() const
{
    return settings.value("volumeIncrement", 2).toInt();
}

void SettingsPage::setVolumeIncrement(int value)
{
    if (value != volumeIncrement()) {
        settings.setValue("volumeIncrement", value);
        emit settingsChanged();
    }
}

bool SettingsPage::mergeSimilarApps() const
{
    return settings.value("mergeSimilarApps", true).toBool();
}

void SettingsPage::setMergeSimilarApps(bool value)
{
    if (value != mergeSimilarApps()) {
        settings.setValue("mergeSimilarApps", value);
        emit settingsChanged();
    }
}

void SettingsPage::showWindow()
{
    QObject *rootObject = engine->rootObjects().isEmpty() ? nullptr : engine->rootObjects().first();
    if (rootObject) {
        rootObject->setProperty("visible", true);
    }
}

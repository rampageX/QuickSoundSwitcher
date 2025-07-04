#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "updatemanager.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    
    if (argc < 3) {
        qDebug() << "Usage: QSSUpdater.exe <download_url> <target_executable>";
        return 1;
    }

    QString downloadUrl = argv[1];
    QString targetPath = argv[2];

    QQmlApplicationEngine engine;
    UpdateManager updateManager;
    
    engine.rootContext()->setContextProperty("updateManager", &updateManager);
    engine.loadFromModule("Odizinne.QSSUpdater", "Main");

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    QObject* window = engine.rootObjects().first();
    
    // Connect signals to update UI
    QObject::connect(&updateManager, &UpdateManager::statusChanged, [window](const QString& status) {
        window->setProperty("statusText", status);
    });
    
    QObject::connect(&updateManager, &UpdateManager::progressChanged, [window](double progress) {
        window->setProperty("progress", progress);
    });
    
    QObject::connect(&updateManager, &UpdateManager::error, [window](const QString& error) {
        window->setProperty("statusText", "Error: " + error);
    });
    
    QObject::connect(&updateManager, &UpdateManager::finished, &app, &QGuiApplication::quit);
    
    // Start update
    updateManager.startUpdate(downloadUrl, targetPath);

    return app.exec();
}

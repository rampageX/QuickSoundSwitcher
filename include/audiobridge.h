// include/audiobridge.h
#ifndef AUDIOBRIDGE_H
#define AUDIOBRIDGE_H

#include <QObject>
#include <QQmlEngine>
#include <QtQml/qqmlregistration.h>
#include <QAbstractListModel>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include "audiomanager.h"

class ApplicationModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum ApplicationRoles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        ExecutableNameRole,
        IconPathRole,
        VolumeRole,
        IsMutedRole,
        AudioLevelRole
    };
    Q_ENUM(ApplicationRoles)

    explicit ApplicationModel(QObject *parent = nullptr);

    // QAbstractListModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Model management
    void setApplications(const QList<AudioApplication>& applications);
    void updateApplicationVolume(const QString& appId, int volume);
    void updateApplicationMute(const QString& appId, bool muted);

private:
    QList<AudioApplication> m_applications;
    int findApplicationIndex(const QString& appId) const;
};

class FilteredDeviceModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int currentDefaultIndex READ getCurrentDefaultIndex NOTIFY currentDefaultIndexChanged)

public:
    enum DeviceRoles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        ShortNameRole,
        DescriptionRole,
        IsDefaultRole,
        IsDefaultCommunicationRole,
        StateRole
    };
    Q_ENUM(DeviceRoles)

    explicit FilteredDeviceModel(bool isInputFilter, QObject *parent = nullptr);

    // QAbstractListModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Model management
    void setDevices(const QList<AudioDevice>& devices);
    Q_INVOKABLE int getCurrentDefaultIndex() const;
    Q_INVOKABLE QString getDeviceName(int index) const;
    Q_INVOKABLE QString getDeviceShortName(int index) const;

signals:
    void currentDefaultIndexChanged();

private:
    QList<AudioDevice> m_devices;
    bool m_isInputFilter;
    int m_currentDefaultIndex;
    int findDeviceIndex(const QString& deviceId) const;
    void updateCurrentDefaultIndex();
};

class AudioBridge : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(int outputVolume READ outputVolume NOTIFY outputVolumeChanged)
    Q_PROPERTY(int inputVolume READ inputVolume NOTIFY inputVolumeChanged)
    Q_PROPERTY(bool outputMuted READ outputMuted NOTIFY outputMutedChanged)
    Q_PROPERTY(bool inputMuted READ inputMuted NOTIFY inputMutedChanged)
    Q_PROPERTY(bool isReady READ isReady NOTIFY isReadyChanged)
    Q_PROPERTY(ApplicationModel* applications READ applications CONSTANT)
    Q_PROPERTY(FilteredDeviceModel* outputDevices READ outputDevices CONSTANT)
    Q_PROPERTY(FilteredDeviceModel* inputDevices READ inputDevices CONSTANT)
    Q_PROPERTY(QVariantList commAppsList READ commAppsList NOTIFY commAppsListChanged)

public:
    explicit AudioBridge(QObject *parent = nullptr);
    ~AudioBridge();
    static AudioBridge* create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    // Properties
    int outputVolume() const { return m_outputVolume; }
    int inputVolume() const { return m_inputVolume; }
    bool outputMuted() const { return m_outputMuted; }
    bool inputMuted() const { return m_inputMuted; }
    bool isReady() const { return m_isReady; }
    ApplicationModel* applications() { return m_applicationModel; }
    FilteredDeviceModel* outputDevices() { return m_outputDeviceModel; }
    FilteredDeviceModel* inputDevices() { return m_inputDeviceModel; }
    QVariantList commAppsList() const;

    // Invokable methods - Volume Control
    Q_INVOKABLE void setOutputVolume(int volume);
    Q_INVOKABLE void setInputVolume(int volume);
    Q_INVOKABLE void setOutputMute(bool mute);
    Q_INVOKABLE void setInputMute(bool mute);
    Q_INVOKABLE void setApplicationVolume(const QString& appId, int volume);
    Q_INVOKABLE void setApplicationMute(const QString& appId, bool mute);

    // Invokable methods - Device Management
    Q_INVOKABLE void setDefaultDevice(const QString& deviceId, bool isInput, bool forCommunications = false);
    Q_INVOKABLE void setOutputDevice(int deviceIndex);
    Q_INVOKABLE void setInputDevice(int deviceIndex);

    // ChatMix methods
    Q_INVOKABLE void applyChatMixToApplications(int value);
    Q_INVOKABLE void restoreOriginalVolumes();
    Q_INVOKABLE bool isCommApp(const QString& name) const;
    Q_INVOKABLE void addCommApp(const QString& name);
    Q_INVOKABLE void removeCommApp(const QString& name);

signals:
    void outputVolumeChanged();
    void inputVolumeChanged();
    void outputMutedChanged();
    void inputMutedChanged();
    void isReadyChanged();
    void deviceAdded(const QString& deviceId, const QString& deviceName);
    void deviceRemoved(const QString& deviceId);
    void defaultDeviceChanged(const QString& deviceId, bool isInput);
    void commAppsListChanged();

private slots:
    void onOutputVolumeChanged(int volume);
    void onInputVolumeChanged(int volume);
    void onOutputMuteChanged(bool muted);
    void onInputMuteChanged(bool muted);
    void onApplicationsChanged(const QList<AudioApplication>& applications);
    void onApplicationVolumeChanged(const QString& appId, int volume);
    void onApplicationMuteChanged(const QString& appId, bool muted);
    void onDevicesChanged(const QList<AudioDevice>& devices);
    void onDeviceAdded(const AudioDevice& device);
    void onDeviceRemoved(const QString& deviceId);
    void onDefaultDeviceChanged(const QString& deviceId, bool isInput);
    void onInitializationComplete();

private:
    int m_outputVolume;
    int m_inputVolume;
    bool m_outputMuted;
    bool m_inputMuted;
    bool m_isReady;
    ApplicationModel* m_applicationModel;
    FilteredDeviceModel* m_outputDeviceModel;
    FilteredDeviceModel* m_inputDeviceModel;

    // ChatMix - no stored values, read from UserSettings
    struct CommApp {
        QString name;
        QString icon;
    };
    QList<CommApp> m_commApps;

    // Helper methods to read from UserSettings
    void applyChatMixIfEnabled();
    QString getCommAppsFilePath() const;
    void loadCommAppsFromFile();
    void saveCommAppsToFile();

    void restoreOriginalVolumesSync();
};

#endif // AUDIOBRIDGE_H

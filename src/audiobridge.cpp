// src/audiobridge.cpp
#include "audiobridge.h"
#include "audiomanager.h"
#include <QDebug>
#include <QQmlContext>
#include <QSettings>
// ApplicationModel implementation
ApplicationModel::ApplicationModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ApplicationModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_applications.count();
}

QVariant ApplicationModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_applications.count())
        return QVariant();

    const AudioApplication& app = m_applications.at(index.row());

    switch (role) {
    case IdRole:
        return app.id;
    case NameRole:
        return app.name;
    case ExecutableNameRole:
        return app.executableName;
    case IconPathRole:
        return app.iconPath;
    case VolumeRole:
        return app.volume;
    case IsMutedRole:
        return app.isMuted;
    case AudioLevelRole:
        return app.audioLevel;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> ApplicationModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IdRole] = "appId";
    roles[NameRole] = "name";
    roles[ExecutableNameRole] = "executableName";
    roles[IconPathRole] = "iconPath";
    roles[VolumeRole] = "volume";
    roles[IsMutedRole] = "isMuted";
    roles[AudioLevelRole] = "audioLevel";
    return roles;
}

void ApplicationModel::updateApplicationAudioLevel(const QString& appId, int level)
{
    int index = findApplicationIndex(appId);
    if (index >= 0) {
        m_applications[index].audioLevel = level;
        QModelIndex modelIndex = createIndex(index, 0);
        emit dataChanged(modelIndex, modelIndex, {AudioLevelRole});
    }
}

void ApplicationModel::setApplications(const QList<AudioApplication>& applications)
{
    beginResetModel();

    QList<AudioApplication> sortedApplications;
    AudioApplication systemSounds;
    bool hasSystemSounds = false;

    for (const AudioApplication& app : applications) {
        if (app.name == "System sounds" || app.id == "system_sounds") {
            systemSounds = app;
            hasSystemSounds = true;
        } else {
            sortedApplications.append(app);
        }
    }

    std::sort(sortedApplications.begin(), sortedApplications.end(),
              [](const AudioApplication& a, const AudioApplication& b) {
                  return a.name.toLower() < b.name.toLower();
              });

    if (hasSystemSounds) {
        sortedApplications.append(systemSounds);
    }

    m_applications = sortedApplications;
    endResetModel();
}

void ApplicationModel::updateApplicationVolume(const QString& appId, int volume)
{
    int index = findApplicationIndex(appId);
    if (index >= 0) {
        m_applications[index].volume = volume;
        QModelIndex modelIndex = createIndex(index, 0);
        emit dataChanged(modelIndex, modelIndex, {VolumeRole});
    }
}

void ApplicationModel::updateApplicationMute(const QString& appId, bool muted)
{
    int index = findApplicationIndex(appId);
    if (index >= 0) {
        m_applications[index].isMuted = muted;
        QModelIndex modelIndex = createIndex(index, 0);
        emit dataChanged(modelIndex, modelIndex, {IsMutedRole});
    }
}

int ApplicationModel::findApplicationIndex(const QString& appId) const
{
    for (int i = 0; i < m_applications.count(); ++i) {
        if (m_applications[i].id == appId) {
            return i;
        }
    }
    return -1;
}

// FilteredDeviceModel implementation
FilteredDeviceModel::FilteredDeviceModel(bool isInputFilter, QObject *parent)
    : QAbstractListModel(parent), m_isInputFilter(isInputFilter), m_currentDefaultIndex(-1)
{
}

int FilteredDeviceModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_devices.count();
}

QVariant FilteredDeviceModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_devices.count())
        return QVariant();

    const AudioDevice& device = m_devices.at(index.row());

    switch (role) {
    case IdRole:
        return device.id;
    case NameRole:
        return device.name;
    case ShortNameRole:
        return device.shortName;
    case DescriptionRole:
        return device.description;
    case IsDefaultRole:
        return device.isDefault;
    case IsDefaultCommunicationRole:
        return device.isDefaultCommunication;
    case StateRole:
        return device.state;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> FilteredDeviceModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IdRole] = "deviceId";
    roles[NameRole] = "name";
    roles[ShortNameRole] = "shortName";
    roles[DescriptionRole] = "description";
    roles[IsDefaultRole] = "isDefault";
    roles[IsDefaultCommunicationRole] = "isDefaultCommunication";
    roles[StateRole] = "state";
    return roles;
}

QString FilteredDeviceModel::getDeviceName(int index) const
{
    if (index >= 0 && index < m_devices.count()) {
        return m_devices[index].name;
    }
    return QString();
}

QString FilteredDeviceModel::getDeviceShortName(int index) const
{
    if (index >= 0 && index < m_devices.count()) {
        return m_devices[index].shortName;
    }
    return QString();
}

void FilteredDeviceModel::setDevices(const QList<AudioDevice>& devices)
{
    beginResetModel();
    m_devices.clear();

    for (const AudioDevice& device : devices) {
        if (device.isInput == m_isInputFilter) {
            m_devices.append(device);
        }
    }

    endResetModel();
    updateCurrentDefaultIndex();
}

int FilteredDeviceModel::getCurrentDefaultIndex() const
{
    return m_currentDefaultIndex;
}

void FilteredDeviceModel::updateCurrentDefaultIndex()
{
    int newDefaultIndex = -1;
    for (int i = 0; i < m_devices.count(); ++i) {
        if (m_devices[i].isDefault) {
            newDefaultIndex = i;
            break;
        }
    }

    if (m_currentDefaultIndex != newDefaultIndex) {
        m_currentDefaultIndex = newDefaultIndex;
        emit currentDefaultIndexChanged();
    }
}

int FilteredDeviceModel::findDeviceIndex(const QString& deviceId) const
{
    for (int i = 0; i < m_devices.count(); ++i) {
        if (m_devices[i].id == deviceId) {
            return i;
        }
    }
    return -1;
}

// AudioBridge implementation
AudioBridge::AudioBridge(QObject *parent)
    : QObject(parent)
    , m_outputVolume(0)
    , m_inputVolume(0)
    , m_outputMuted(false)
    , m_inputMuted(false)
    , m_isReady(false)
    , m_applicationModel(new ApplicationModel(this))
    , m_outputDeviceModel(new FilteredDeviceModel(false, this))
    , m_inputDeviceModel(new FilteredDeviceModel(true, this))
{
    auto* manager = AudioManager::instance();

    // Connect volume signals
    connect(manager, &AudioManager::outputVolumeChanged, this, &AudioBridge::onOutputVolumeChanged);
    connect(manager, &AudioManager::inputVolumeChanged, this, &AudioBridge::onInputVolumeChanged);
    connect(manager, &AudioManager::outputMuteChanged, this, &AudioBridge::onOutputMuteChanged);
    connect(manager, &AudioManager::inputMuteChanged, this, &AudioBridge::onInputMuteChanged);

    // Connect application signals
    connect(manager, &AudioManager::applicationsChanged, this, &AudioBridge::onApplicationsChanged);
    connect(manager, &AudioManager::applicationVolumeChanged, this, &AudioBridge::onApplicationVolumeChanged);
    connect(manager, &AudioManager::applicationMuteChanged, this, &AudioBridge::onApplicationMuteChanged);

    // Connect device signals
    connect(manager, &AudioManager::devicesChanged, this, &AudioBridge::onDevicesChanged);
    connect(manager, &AudioManager::deviceAdded, this, &AudioBridge::onDeviceAdded);
    connect(manager, &AudioManager::deviceRemoved, this, &AudioBridge::onDeviceRemoved);
    connect(manager, &AudioManager::defaultDeviceChanged, this, &AudioBridge::onDefaultDeviceChanged);

    connect(manager, &AudioManager::initializationComplete, this, &AudioBridge::onInitializationComplete);

    connect(manager, &AudioManager::outputAudioLevelChanged, this, &AudioBridge::onOutputAudioLevelChanged);
    connect(manager, &AudioManager::inputAudioLevelChanged, this, &AudioBridge::onInputAudioLevelChanged);
    connect(manager, &AudioManager::applicationAudioLevelChanged, this, &AudioBridge::onApplicationAudioLevelChanged);

    connect(this, &AudioBridge::applicationAudioLevelChanged,
            m_applicationModel, &ApplicationModel::updateApplicationAudioLevel);

    // Load comm apps
    loadCommAppsFromFile();

    // Initialize the manager
    manager->initialize();
}

AudioBridge::~AudioBridge() {
    QSettings settings("Odizinne", "QuickSoundSwitcher");
    bool activateChatMix = settings.value("activateChatmix").toBool();
    bool chatMixEnabled = settings.value("chatMixEnabled").toBool();

    if (activateChatMix && chatMixEnabled) {
        restoreOriginalVolumesSync();
    }

    auto* manager = AudioManager::instance();
    manager->cleanup();
}

AudioBridge* AudioBridge::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
{
    Q_UNUSED(qmlEngine)
    Q_UNUSED(jsEngine)
    return new AudioBridge();
}

// Volume control methods
void AudioBridge::setOutputVolume(int volume)
{
    AudioManager::instance()->setOutputVolumeAsync(volume);
}

void AudioBridge::setInputVolume(int volume)
{
    AudioManager::instance()->setInputVolumeAsync(volume);
}

void AudioBridge::setOutputMute(bool mute)
{
    AudioManager::instance()->setOutputMuteAsync(mute);
}

void AudioBridge::setInputMute(bool mute)
{
    AudioManager::instance()->setInputMuteAsync(mute);
}

void AudioBridge::setApplicationVolume(const QString& appId, int volume)
{
    AudioManager::instance()->setApplicationVolumeAsync(appId, volume);
}

void AudioBridge::setApplicationMute(const QString& appId, bool mute)
{
    AudioManager::instance()->setApplicationMuteAsync(appId, mute);
}

// Device management methods
void AudioBridge::setDefaultDevice(const QString& deviceId, bool isInput, bool forCommunications)
{
    AudioManager::instance()->setDefaultDeviceAsync(deviceId, isInput, forCommunications);
}

void AudioBridge::setOutputDevice(int deviceIndex)
{
    if (deviceIndex >= 0 && deviceIndex < m_outputDeviceModel->rowCount()) {
        QModelIndex modelIndex = m_outputDeviceModel->index(deviceIndex);
        QString deviceId = m_outputDeviceModel->data(modelIndex, FilteredDeviceModel::IdRole).toString();

        setDefaultDevice(deviceId, false, false);
        setDefaultDevice(deviceId, false, true);
    }
}

void AudioBridge::setInputDevice(int deviceIndex)
{
    if (deviceIndex >= 0 && deviceIndex < m_inputDeviceModel->rowCount()) {
        QModelIndex modelIndex = m_inputDeviceModel->index(deviceIndex);
        QString deviceId = m_inputDeviceModel->data(modelIndex, FilteredDeviceModel::IdRole).toString();

        setDefaultDevice(deviceId, true, false);
        setDefaultDevice(deviceId, true, true);
    }
}

// ChatMix methods
void AudioBridge::applyChatMixToApplications(int value)
{
    for (int i = 0; i < m_applicationModel->rowCount(); ++i) {
        QModelIndex index = m_applicationModel->index(i, 0);
        QString appId = m_applicationModel->data(index, ApplicationModel::IdRole).toString();
        QString appName = m_applicationModel->data(index, ApplicationModel::NameRole).toString();

        if (appName == "System sounds" || appId == "system_sounds") {
            continue;
        }

        int targetVolume = isCommApp(appName) ? 100 : value;
        setApplicationVolume(appId, targetVolume);
    }
}

void AudioBridge::restoreOriginalVolumes()
{
    QSettings settings("Odizinne", "QuickSoundSwitcher");
    int restoreVolume = settings.value("chatmixRestoreVolume").toInt();

    if (!m_isReady) {
        return;
    }

    for (int i = 0; i < m_applicationModel->rowCount(); ++i) {
        QModelIndex index = m_applicationModel->index(i, 0);
        QString appId = m_applicationModel->data(index, ApplicationModel::IdRole).toString();
        QString appName = m_applicationModel->data(index, ApplicationModel::NameRole).toString();

        if (appName == "System sounds" || appId == "system_sounds") {
            continue;
        }

        setApplicationVolume(appId, restoreVolume);
    }
}

void AudioBridge::applyChatMixIfEnabled()
{
    QSettings settings("Odizinne", "QuickSoundSwitcher");
    bool activateChatMix = settings.value("activateChatmix").toBool();
    bool chatMixEnabled = settings.value("chatMixEnabled").toBool();

    if (activateChatMix && chatMixEnabled) {
        QSettings settings("Odizinne", "QuickSoundSwitcher");
        applyChatMixToApplications(settings.value("chatMixValue").toInt());
    }
}

bool AudioBridge::isCommApp(const QString& name) const
{
    for (const CommApp& app : m_commApps) {
        if (app.name.compare(name, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}

void AudioBridge::addCommApp(const QString& name)
{
    for (const CommApp& existing : m_commApps) {
        if (existing.name.compare(name, Qt::CaseInsensitive) == 0) {
            return;
        }
    }

    CommApp newApp;
    newApp.name = name;

    for (int i = 0; i < m_applicationModel->rowCount(); ++i) {
        QModelIndex index = m_applicationModel->index(i, 0);
        QString appName = m_applicationModel->data(index, ApplicationModel::NameRole).toString();

        if (appName.compare(name, Qt::CaseInsensitive) == 0) {
            newApp.icon = m_applicationModel->data(index, ApplicationModel::IconPathRole).toString();
            break;
        }
    }

    m_commApps.append(newApp);
    saveCommAppsToFile();
    emit commAppsListChanged();
}

void AudioBridge::removeCommApp(const QString& name)
{
    for (int i = 0; i < m_commApps.count(); ++i) {
        if (m_commApps[i].name.compare(name, Qt::CaseInsensitive) == 0) {
            m_commApps.removeAt(i);
            saveCommAppsToFile();
            emit commAppsListChanged();
            return;
        }
    }
}

QVariantList AudioBridge::commAppsList() const
{
    QVariantList result;
    for (const CommApp& app : m_commApps) {
        QVariantMap appMap;
        appMap["name"] = app.name;
        appMap["icon"] = app.icon;
        result.append(appMap);
    }
    return result;
}

QString AudioBridge::getCommAppsFilePath() const
{
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appDataPath);
    return appDataPath + "/commapps.json";
}

void AudioBridge::loadCommAppsFromFile()
{
    QString filePath = getCommAppsFilePath();
    QFile file(filePath);

    if (!file.exists()) {
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QByteArray data = file.readAll();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        return;
    }

    QJsonObject root = doc.object();
    QJsonArray commAppsArray = root["commApps"].toArray();

    m_commApps.clear();
    for (const QJsonValue& value : commAppsArray) {
        QJsonObject appObj = value.toObject();
        CommApp app;
        app.name = appObj["name"].toString();
        app.icon = appObj["icon"].toString();
        m_commApps.append(app);
    }
}

void AudioBridge::saveCommAppsToFile()
{
    QString filePath = getCommAppsFilePath();
    QFile file(filePath);

    if (!file.open(QIODevice::WriteOnly)) {
        return;
    }

    QJsonArray commAppsArray;
    for (const CommApp& app : m_commApps) {
        QJsonObject appObj;
        appObj["name"] = app.name;
        appObj["icon"] = app.icon;
        commAppsArray.append(appObj);
    }

    QJsonObject root;
    root["commApps"] = commAppsArray;

    QJsonDocument doc(root);
    file.write(doc.toJson());
}

// Event handlers
void AudioBridge::onOutputVolumeChanged(int volume)
{
    if (m_outputVolume != volume) {
        m_outputVolume = volume;
        emit outputVolumeChanged();
    }
}

void AudioBridge::onInputVolumeChanged(int volume)
{
    if (m_inputVolume != volume) {
        m_inputVolume = volume;
        emit inputVolumeChanged();
    }
}

void AudioBridge::onOutputMuteChanged(bool muted)
{
    if (m_outputMuted != muted) {
        m_outputMuted = muted;
        emit outputMutedChanged();
    }
}

void AudioBridge::onInputMuteChanged(bool muted)
{
    if (m_inputMuted != muted) {
        m_inputMuted = muted;
        emit inputMutedChanged();
    }
}

void AudioBridge::onApplicationVolumeChanged(const QString& appId, int volume)
{
    m_applicationModel->updateApplicationVolume(appId, volume);
}

void AudioBridge::onApplicationMuteChanged(const QString& appId, bool muted)
{
    m_applicationModel->updateApplicationMute(appId, muted);
}

void AudioBridge::onApplicationsChanged(const QList<AudioApplication>& applications)
{
    m_applicationModel->setApplications(applications);

    QTimer::singleShot(0, this, &AudioBridge::applyChatMixIfEnabled);
}

void AudioBridge::onDevicesChanged(const QList<AudioDevice>& devices)
{
    m_outputDeviceModel->setDevices(devices);
    m_inputDeviceModel->setDevices(devices);
}

void AudioBridge::onDeviceAdded(const AudioDevice& device)
{
    emit deviceAdded(device.id, device.name);
}

void AudioBridge::onDeviceRemoved(const QString& deviceId)
{
    emit deviceRemoved(deviceId);
}

void AudioBridge::onDefaultDeviceChanged(const QString& deviceId, bool isInput)
{
    emit defaultDeviceChanged(deviceId, isInput);
}

void AudioBridge::onInitializationComplete()
{
    auto* manager = AudioManager::instance();
    m_outputVolume = manager->getOutputVolume();
    m_inputVolume = manager->getInputVolume();
    m_outputMuted = manager->getOutputMute();
    m_inputMuted = manager->getInputMute();

    QList<AudioApplication> apps = manager->getApplications();
    m_applicationModel->setApplications(apps);

    QList<AudioDevice> devices = manager->getDevices();
    m_outputDeviceModel->setDevices(devices);
    m_inputDeviceModel->setDevices(devices);

    m_isReady = true;

    emit outputVolumeChanged();
    emit inputVolumeChanged();
    emit outputMutedChanged();
    emit inputMutedChanged();
    emit isReadyChanged();

    applyChatMixIfEnabled();
}

void AudioBridge::restoreOriginalVolumesSync()
{
    QSettings settings("Odizinne", "QuickSoundSwitcher");
    int restoreVolume = settings.value("chatmixRestoreVolume").toInt();

    if (!m_isReady) {
        return;
    }

    auto* worker = AudioManager::instance()->getWorker();
    if (!worker) return;

    for (int i = 0; i < m_applicationModel->rowCount(); ++i) {
        QModelIndex index = m_applicationModel->index(i, 0);
        QString appId = m_applicationModel->data(index, ApplicationModel::IdRole).toString();
        QString appName = m_applicationModel->data(index, ApplicationModel::NameRole).toString();

        if (appName == "System sounds" || appId == "system_sounds") {
            continue;
        }

        // Use blocking call to ensure completion before destructor finishes
        QMetaObject::invokeMethod(worker, "setApplicationVolume",
                                  Qt::BlockingQueuedConnection,
                                  Q_ARG(QString, appId),
                                  Q_ARG(int, restoreVolume));
    }
}

void AudioBridge::startAudioLevelMonitoring()
{
    AudioManager::instance()->startAudioLevelMonitoring();
}

void AudioBridge::stopAudioLevelMonitoring()
{
    AudioManager::instance()->stopAudioLevelMonitoring();
}

void AudioBridge::onOutputAudioLevelChanged(int level)
{
    if (m_outputAudioLevel != level) {
        m_outputAudioLevel = level;
        emit outputAudioLevelChanged();
    }
}

void AudioBridge::onInputAudioLevelChanged(int level)
{
    if (m_inputAudioLevel != level) {
        m_inputAudioLevel = level;
        emit inputAudioLevelChanged();
    }
}

void AudioBridge::onApplicationAudioLevelChanged(const QString& appId, int level)
{
    m_applicationAudioLevels[appId] = level;

    // Update the application model
    for (int i = 0; i < m_applicationModel->rowCount(); ++i) {
        QModelIndex index = m_applicationModel->index(i, 0);
        QString modelAppId = m_applicationModel->data(index, ApplicationModel::IdRole).toString();

        if (modelAppId == appId) {
            // Update the model with new audio level
            // You might need to add this to your ApplicationModel
            emit applicationAudioLevelChanged(appId, level);
            break;
        }
    }
}

int AudioBridge::getApplicationAudioLevel(const QString& appId) const
{
    return m_applicationAudioLevels.value(appId, 0);
}

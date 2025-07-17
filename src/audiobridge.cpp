#include "audiobridge.h"
#include "audiomanager.h"
#include <QDebug>

// ApplicationModel implementation (unchanged)
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

void ApplicationModel::setApplications(const QList<AudioApplication>& applications)
{
    beginResetModel();

    // Create a copy to work with
    QList<AudioApplication> sortedApplications;
    AudioApplication systemSounds;
    bool hasSystemSounds = false;

    // Separate system sounds from other applications
    for (const AudioApplication& app : applications) {
        if (app.name == "System sounds" || app.id == "system_sounds") {
            systemSounds = app;
            hasSystemSounds = true;
        } else {
            sortedApplications.append(app);
        }
    }

    // Sort regular applications alphabetically by name (case-insensitive)
    std::sort(sortedApplications.begin(), sortedApplications.end(),
              [](const AudioApplication& a, const AudioApplication& b) {
                  return a.name.toLower() < b.name.toLower();
              });

    // Add system sounds at the end (should always be present now)
    if (hasSystemSounds) {
        sortedApplications.append(systemSounds);
    }

    m_applications = sortedApplications;
    endResetModel();

    qDebug() << "ApplicationModel updated with" << applications.count() << "applications (sorted alphabetically, system sounds last)";
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
    roles[DescriptionRole] = "description";
    roles[IsDefaultRole] = "isDefault";
    roles[IsDefaultCommunicationRole] = "isDefaultCommunication";
    roles[StateRole] = "state";
    return roles;
}

void FilteredDeviceModel::setDevices(const QList<AudioDevice>& devices)
{
    beginResetModel();
    m_devices.clear();

    // Filter devices based on input/output
    for (const AudioDevice& device : devices) {
        if (device.isInput == m_isInputFilter) {
            m_devices.append(device);
        }
    }

    endResetModel();
    updateCurrentDefaultIndex();
    qDebug() << "FilteredDeviceModel updated with" << m_devices.count() << (m_isInputFilter ? "input" : "output") << "devices";
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
        qDebug() << "Default index changed to" << m_currentDefaultIndex << "for" << (m_isInputFilter ? "input" : "output") << "devices";
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
    , m_outputDeviceModel(new FilteredDeviceModel(false, this)) // false = output devices
    , m_inputDeviceModel(new FilteredDeviceModel(true, this))   // true = input devices
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

    // Initialize the manager
    manager->initialize();
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
    qDebug() << "AudioBridge::setApplicationVolume" << appId << volume;
    AudioManager::instance()->setApplicationVolumeAsync(appId, volume);
}

void AudioBridge::setApplicationMute(const QString& appId, bool mute)
{
    qDebug() << "AudioBridge::setApplicationMute" << appId << mute;
    AudioManager::instance()->setApplicationMuteAsync(appId, mute);
}

// Device management methods
void AudioBridge::setDefaultDevice(const QString& deviceId, bool isInput, bool forCommunications)
{
    qDebug() << "AudioBridge::setDefaultDevice" << deviceId << "isInput:" << isInput << "forComm:" << forCommunications;
    AudioManager::instance()->setDefaultDeviceAsync(deviceId, isInput, forCommunications);
}

void AudioBridge::setOutputDevice(int deviceIndex)
{
    if (deviceIndex >= 0 && deviceIndex < m_outputDeviceModel->rowCount()) {
        QModelIndex modelIndex = m_outputDeviceModel->index(deviceIndex);
        QString deviceId = m_outputDeviceModel->data(modelIndex, FilteredDeviceModel::IdRole).toString();

        qDebug() << "AudioBridge::setOutputDevice" << deviceIndex << deviceId;

        // Set both default and communication roles
        setDefaultDevice(deviceId, false, false);
        setDefaultDevice(deviceId, false, true);
    }
}

void AudioBridge::setInputDevice(int deviceIndex)
{
    if (deviceIndex >= 0 && deviceIndex < m_inputDeviceModel->rowCount()) {
        QModelIndex modelIndex = m_inputDeviceModel->index(deviceIndex);
        QString deviceId = m_inputDeviceModel->data(modelIndex, FilteredDeviceModel::IdRole).toString();

        qDebug() << "AudioBridge::setInputDevice" << deviceIndex << deviceId;

        // Set both default and communication roles
        setDefaultDevice(deviceId, true, false);
        setDefaultDevice(deviceId, true, true);
    }
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
    qDebug() << "AudioBridge::onApplicationVolumeChanged" << appId << volume;
    m_applicationModel->updateApplicationVolume(appId, volume);
}

void AudioBridge::onApplicationMuteChanged(const QString& appId, bool muted)
{
    qDebug() << "AudioBridge::onApplicationMuteChanged" << appId << muted;
    m_applicationModel->updateApplicationMute(appId, muted);
}

void AudioBridge::onApplicationsChanged(const QList<AudioApplication>& applications)
{
    qDebug() << "AudioBridge::onApplicationsChanged - received" << applications.count() << "applications";
    for (const auto& app : applications) {
        qDebug() << "  App:" << app.name << "Volume:" << app.volume << "Muted:" << app.isMuted;
    }
    m_applicationModel->setApplications(applications);
}

void AudioBridge::onDevicesChanged(const QList<AudioDevice>& devices)
{
    qDebug() << "AudioBridge::onDevicesChanged - received" << devices.count() << "devices";
    for (const auto& device : devices) {
        qDebug() << "  Device:" << device.name << "Input:" << device.isInput << "Default:" << device.isDefault;
    }

    // Update both filtered models
    m_outputDeviceModel->setDevices(devices);
    m_inputDeviceModel->setDevices(devices);
}

void AudioBridge::onDeviceAdded(const AudioDevice& device)
{
    qDebug() << "AudioBridge::onDeviceAdded" << device.name;
    emit deviceAdded(device.id, device.name);
}

void AudioBridge::onDeviceRemoved(const QString& deviceId)
{
    qDebug() << "AudioBridge::onDeviceRemoved" << deviceId;
    emit deviceRemoved(deviceId);
}

void AudioBridge::onDefaultDeviceChanged(const QString& deviceId, bool isInput)
{
    qDebug() << "AudioBridge::onDefaultDeviceChanged" << deviceId << "isInput:" << isInput;
    emit defaultDeviceChanged(deviceId, isInput);
}

void AudioBridge::onInitializationComplete()
{
    // Get initial values
    auto* manager = AudioManager::instance();
    m_outputVolume = manager->getOutputVolume();
    m_inputVolume = manager->getInputVolume();
    m_outputMuted = manager->getOutputMute();
    m_inputMuted = manager->getInputMute();

    // Set initial applications and devices
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

    qDebug() << "AudioBridge initialization complete";
}

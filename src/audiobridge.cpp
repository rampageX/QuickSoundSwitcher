// src/audiobridge.cpp
#include "audiobridge.h"
#include "audiomanager.h"
#include <QDebug>
#include <QQmlContext>
#include <QSettings>

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

// FilteredDeviceModel implementation (unchanged - keeping existing code)
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

// GroupedApplicationModel implementation
GroupedApplicationModel::GroupedApplicationModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int GroupedApplicationModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_groups.count();
}

QVariant GroupedApplicationModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_groups.count())
        return QVariant();

    const ApplicationGroup& group = m_groups.at(index.row());

    switch (role) {
    case ExecutableNameRole:
        return group.executableName;
    case DisplayNameRole:
        return group.displayName;
    case IconPathRole:
        return group.iconPath;
    case AverageVolumeRole:
        return group.averageVolume;
    case AnyMutedRole:
        return group.anyMuted;
    case AllMutedRole:
        return group.allMuted;
    case SessionCountRole:
        return group.sessionCount;
    default:
        return QVariant();
    }
}

void GroupedApplicationModel::updateGroupVolume(const QString& executableName, int averageVolume)
{
    for (int i = 0; i < m_groups.count(); ++i) {
        if (m_groups[i].executableName == executableName) {
            m_groups[i].averageVolume = averageVolume;
            QModelIndex modelIndex = createIndex(i, 0);
            emit dataChanged(modelIndex, modelIndex, {AverageVolumeRole});
            break;
        }
    }
}

void GroupedApplicationModel::updateGroupMute(const QString& executableName, bool anyMuted, bool allMuted)
{
    for (int i = 0; i < m_groups.count(); ++i) {
        if (m_groups[i].executableName == executableName) {
            m_groups[i].anyMuted = anyMuted;
            m_groups[i].allMuted = allMuted;
            QModelIndex modelIndex = createIndex(i, 0);
            emit dataChanged(modelIndex, modelIndex, {AnyMutedRole, AllMutedRole});
            break;
        }
    }
}

QHash<int, QByteArray> GroupedApplicationModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[ExecutableNameRole] = "executableName";
    roles[DisplayNameRole] = "displayName";
    roles[IconPathRole] = "iconPath";
    roles[AverageVolumeRole] = "averageVolume";
    roles[AnyMutedRole] = "anyMuted";
    roles[AllMutedRole] = "allMuted";
    roles[SessionCountRole] = "sessionCount";
    return roles;
}

void GroupedApplicationModel::setGroups(const QList<ApplicationGroup>& groups)
{
    beginResetModel();
    m_groups = groups;
    endResetModel();
}

// ExecutableSessionModel implementation
ExecutableSessionModel::ExecutableSessionModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ExecutableSessionModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_sessions.count();
}

QVariant ExecutableSessionModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_sessions.count())
        return QVariant();

    const AudioApplication& session = m_sessions.at(index.row());

    switch (role) {
    case IdRole:
        return session.id;
    case NameRole:
        return session.name;
    case ExecutableNameRole:
        return session.executableName;
    case IconPathRole:
        return session.iconPath;
    case VolumeRole:
        return session.volume;
    case IsMutedRole:
        return session.isMuted;
    case AudioLevelRole:
        return session.audioLevel;
    default:
        return QVariant();
    }
}

void ExecutableSessionModel::updateSessionVolume(const QString& appId, int volume)
{
    for (int i = 0; i < m_sessions.count(); ++i) {
        if (m_sessions[i].id == appId) {
            m_sessions[i].volume = volume;
            QModelIndex modelIndex = createIndex(i, 0);
            emit dataChanged(modelIndex, modelIndex, {VolumeRole});
            break;
        }
    }
}

void ExecutableSessionModel::updateSessionMute(const QString& appId, bool muted)
{
    for (int i = 0; i < m_sessions.count(); ++i) {
        if (m_sessions[i].id == appId) {
            m_sessions[i].isMuted = muted;
            QModelIndex modelIndex = createIndex(i, 0);
            emit dataChanged(modelIndex, modelIndex, {IsMutedRole});
            break;
        }
    }
}

QHash<int, QByteArray> ExecutableSessionModel::roleNames() const
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

void ExecutableSessionModel::setSessions(const QList<AudioApplication>& sessions)
{
    beginResetModel();
    m_sessions = sessions;
    endResetModel();
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
    , m_groupedApplicationModel(new GroupedApplicationModel(this))
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

    // Clean up session models
    for (auto* model : m_sessionModels) {
        delete model;
    }
    m_sessionModels.clear();

    auto* manager = AudioManager::instance();
    manager->cleanup();
}

AudioBridge* AudioBridge::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
{
    Q_UNUSED(qmlEngine)
    Q_UNUSED(jsEngine)
    return new AudioBridge();
}

// Grouped application methods
ExecutableSessionModel* AudioBridge::getSessionsForExecutable(const QString& executableName)
{
    if (!m_sessionModels.contains(executableName)) {
        m_sessionModels[executableName] = new ExecutableSessionModel(this);
    }

    ExecutableSessionModel* model = m_sessionModels[executableName];

    // Find sessions for this executable
    QList<AudioApplication> sessions;
    for (int i = 0; i < m_applicationModel->rowCount(); ++i) {
        QModelIndex index = m_applicationModel->index(i, 0);
        QString appExecutableName = m_applicationModel->data(index, ApplicationModel::ExecutableNameRole).toString();

        if (appExecutableName == executableName) {
            AudioApplication app;
            app.id = m_applicationModel->data(index, ApplicationModel::IdRole).toString();
            app.name = m_applicationModel->data(index, ApplicationModel::NameRole).toString();
            app.executableName = appExecutableName;
            app.iconPath = m_applicationModel->data(index, ApplicationModel::IconPathRole).toString();
            app.volume = m_applicationModel->data(index, ApplicationModel::VolumeRole).toInt();
            app.isMuted = m_applicationModel->data(index, ApplicationModel::IsMutedRole).toBool();
            app.audioLevel = m_applicationModel->data(index, ApplicationModel::AudioLevelRole).toInt();
            sessions.append(app);
        }
    }

    model->setSessions(sessions);
    return model;
}

void AudioBridge::setExecutableVolume(const QString& executableName, int volume)
{
    // Temporarily disable individual update signals to avoid loops
    bool wasBlocked = blockSignals(true);

    // Set volume for all sessions of this executable
    for (int i = 0; i < m_applicationModel->rowCount(); ++i) {
        QModelIndex index = m_applicationModel->index(i, 0);
        QString appExecutableName = m_applicationModel->data(index, ApplicationModel::ExecutableNameRole).toString();

        if (appExecutableName == executableName) {
            QString appId = m_applicationModel->data(index, ApplicationModel::IdRole).toString();
            setApplicationVolume(appId, volume);
        }
    }

    blockSignals(wasBlocked);

    // Update models directly without triggering the change handlers
    m_groupedApplicationModel->updateGroupVolume(executableName, volume);

    // Update session model
    if (m_sessionModels.contains(executableName)) {
        for (int i = 0; i < m_applicationModel->rowCount(); ++i) {
            QModelIndex index = m_applicationModel->index(i, 0);
            QString appExecutableName = m_applicationModel->data(index, ApplicationModel::ExecutableNameRole).toString();

            if (appExecutableName == executableName) {
                QString appId = m_applicationModel->data(index, ApplicationModel::IdRole).toString();
                m_sessionModels[executableName]->updateSessionVolume(appId, volume);
            }
        }
    }
}

void AudioBridge::setExecutableMute(const QString& executableName, bool muted)
{
    // Set mute for all sessions of this executable
    for (int i = 0; i < m_applicationModel->rowCount(); ++i) {
        QModelIndex index = m_applicationModel->index(i, 0);
        QString appExecutableName = m_applicationModel->data(index, ApplicationModel::ExecutableNameRole).toString();

        if (appExecutableName == executableName) {
            QString appId = m_applicationModel->data(index, ApplicationModel::IdRole).toString();
            setApplicationMute(appId, muted);
        }
    }

    // Update grouped model in place
    m_groupedApplicationModel->updateGroupMute(executableName, muted, muted);

    // Update session model
    if (m_sessionModels.contains(executableName)) {
        for (int i = 0; i < m_applicationModel->rowCount(); ++i) {
            QModelIndex index = m_applicationModel->index(i, 0);
            QString appExecutableName = m_applicationModel->data(index, ApplicationModel::ExecutableNameRole).toString();

            if (appExecutableName == executableName) {
                QString appId = m_applicationModel->data(index, ApplicationModel::IdRole).toString();
                m_sessionModels[executableName]->updateSessionMute(appId, muted);
            }
        }
    }
}

void AudioBridge::updateGroupedApplications()
{
    QMap<QString, ApplicationGroup> groups;

    // Group applications by executable name
    for (int i = 0; i < m_applicationModel->rowCount(); ++i) {
        QModelIndex index = m_applicationModel->index(i, 0);

        QString executableName = m_applicationModel->data(index, ApplicationModel::ExecutableNameRole).toString();
        QString name = m_applicationModel->data(index, ApplicationModel::NameRole).toString();
        QString iconPath = m_applicationModel->data(index, ApplicationModel::IconPathRole).toString();
        int volume = m_applicationModel->data(index, ApplicationModel::VolumeRole).toInt();
        bool muted = m_applicationModel->data(index, ApplicationModel::IsMutedRole).toBool();
        QString appId = m_applicationModel->data(index, ApplicationModel::IdRole).toString();
        int audioLevel = m_applicationModel->data(index, ApplicationModel::AudioLevelRole).toInt();

        AudioApplication app;
        app.id = appId;
        app.name = name;
        app.executableName = executableName;
        app.iconPath = iconPath;
        app.volume = volume;
        app.isMuted = muted;
        app.audioLevel = audioLevel;

        if (!groups.contains(executableName)) {
            ApplicationGroup group;
            group.executableName = executableName;
            group.displayName = executableName;
            group.iconPath = iconPath;
            group.sessions.append(app);
            groups[executableName] = group;
        } else {
            groups[executableName].sessions.append(app);
        }
    }

    // Calculate group statistics
    QList<ApplicationGroup> groupList;
    for (auto& group : groups) {
        int totalVolume = 0;
        int mutedCount = 0;

        for (const auto& app : group.sessions) {
            totalVolume += app.volume;
            if (app.isMuted) mutedCount++;
        }

        group.averageVolume = group.sessions.isEmpty() ? 0 : totalVolume / group.sessions.count();
        group.anyMuted = mutedCount > 0;
        group.allMuted = mutedCount == group.sessions.count();
        group.sessionCount = group.sessions.count();

        groupList.append(group);
    }

    // Sort groups (System sounds last, others alphabetically)
    std::sort(groupList.begin(), groupList.end(),
              [](const ApplicationGroup& a, const ApplicationGroup& b) {
                  if (a.executableName == "System sounds") return false;
                  if (b.executableName == "System sounds") return true;
                  return a.displayName.toLower() < b.displayName.toLower();
              });

    m_groupedApplicationModel->setGroups(groupList);

    // Update session models for each executable
    for (const auto& group : groupList) {
        if (m_sessionModels.contains(group.executableName)) {
            m_sessionModels[group.executableName]->setSessions(group.sessions);
        }
    }
}

// Volume control methods (unchanged)
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

// Device management methods (unchanged)
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

// ChatMix methods (unchanged)
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
    updateGroupForApplication(appId); // Use targeted update instead of full rebuild
}

void AudioBridge::onApplicationMuteChanged(const QString& appId, bool muted)
{
    m_applicationModel->updateApplicationMute(appId, muted);
    updateGroupForApplication(appId); // Use targeted update instead of full rebuild
}

void AudioBridge::onApplicationsChanged(const QList<AudioApplication>& applications)
{
    m_applicationModel->setApplications(applications);
    updateGroupedApplications(); // Only rebuild when apps are added/removed

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
    updateGroupedApplications(); // Initial grouping

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
            emit applicationAudioLevelChanged(appId, level);
            break;
        }
    }
}

int AudioBridge::getApplicationAudioLevel(const QString& appId) const
{
    return m_applicationAudioLevels.value(appId, 0);
}

void AudioBridge::updateGroupForApplication(const QString& appId)
{
    // Find the executable name for this appId
    QString executableName;
    for (int i = 0; i < m_applicationModel->rowCount(); ++i) {
        QModelIndex index = m_applicationModel->index(i, 0);
        if (m_applicationModel->data(index, ApplicationModel::IdRole).toString() == appId) {
            executableName = m_applicationModel->data(index, ApplicationModel::ExecutableNameRole).toString();
            break;
        }
    }

    if (executableName.isEmpty()) return;

    // Update the session model for this executable
    if (m_sessionModels.contains(executableName)) {
        // Find the session and update it
        for (int i = 0; i < m_applicationModel->rowCount(); ++i) {
            QModelIndex index = m_applicationModel->index(i, 0);
            if (m_applicationModel->data(index, ApplicationModel::IdRole).toString() == appId) {
                int volume = m_applicationModel->data(index, ApplicationModel::VolumeRole).toInt();
                bool muted = m_applicationModel->data(index, ApplicationModel::IsMutedRole).toBool();

                m_sessionModels[executableName]->updateSessionVolume(appId, volume);
                m_sessionModels[executableName]->updateSessionMute(appId, muted);
                break;
            }
        }
    }

    // Calculate new group statistics
    int totalVolume = 0;
    int mutedCount = 0;
    int sessionCount = 0;

    for (int i = 0; i < m_applicationModel->rowCount(); ++i) {
        QModelIndex index = m_applicationModel->index(i, 0);
        QString appExecutableName = m_applicationModel->data(index, ApplicationModel::ExecutableNameRole).toString();

        if (appExecutableName == executableName) {
            totalVolume += m_applicationModel->data(index, ApplicationModel::VolumeRole).toInt();
            if (m_applicationModel->data(index, ApplicationModel::IsMutedRole).toBool()) {
                mutedCount++;
            }
            sessionCount++;
        }
    }

    if (sessionCount > 0) {
        int averageVolume = totalVolume / sessionCount;
        bool anyMuted = mutedCount > 0;
        bool allMuted = mutedCount == sessionCount;

        // Update the grouped model in place
        m_groupedApplicationModel->updateGroupVolume(executableName, averageVolume);
        m_groupedApplicationModel->updateGroupMute(executableName, anyMuted, allMuted);
    }
}

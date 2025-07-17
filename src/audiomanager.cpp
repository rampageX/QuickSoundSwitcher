#include "audiomanager.h"
#include <QDebug>
#include <QMutexLocker>
#include <QMetaObject>
#include <QBuffer>
#include <QPixmap>
#include <QFileInfo>
#include <atlbase.h>
#include <psapi.h>
#include <Shlobj.h>
#include <winver.h>
#include <Functiondiscoverykeys_devpkey.h>
#pragma comment(lib, "version.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "shell32.lib")

AudioManager* AudioManager::m_instance = nullptr;
QMutex AudioManager::m_mutex;

// AudioDeviceModel implementation
AudioDeviceModel::AudioDeviceModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int AudioDeviceModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_devices.count();
}

QVariant AudioDeviceModel::data(const QModelIndex &index, int role) const
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
    case IsInputRole:
        return device.isInput;
    case StateRole:
        return device.state;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> AudioDeviceModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IdRole] = "deviceId";
    roles[NameRole] = "name";
    roles[DescriptionRole] = "description";
    roles[IsDefaultRole] = "isDefault";
    roles[IsDefaultCommunicationRole] = "isDefaultCommunication";
    roles[IsInputRole] = "isInput";
    roles[StateRole] = "state";
    return roles;
}

void AudioDeviceModel::setDevices(const QList<AudioDevice>& devices)
{
    beginResetModel();
    m_devices = devices;
    endResetModel();
    qDebug() << "AudioDeviceModel updated with" << devices.count() << "devices";
}

void AudioDeviceModel::updateDevice(const AudioDevice& device)
{
    int index = findDeviceIndex(device.id);
    if (index >= 0) {
        m_devices[index] = device;
        QModelIndex modelIndex = createIndex(index, 0);
        emit dataChanged(modelIndex, modelIndex);
    }
}

void AudioDeviceModel::removeDevice(const QString& deviceId)
{
    int index = findDeviceIndex(deviceId);
    if (index >= 0) {
        beginRemoveRows(QModelIndex(), index, index);
        m_devices.removeAt(index);
        endRemoveRows();
    }
}

int AudioDeviceModel::findDeviceIndex(const QString& deviceId) const
{
    for (int i = 0; i < m_devices.count(); ++i) {
        if (m_devices[i].id == deviceId) {
            return i;
        }
    }
    return -1;
}

// DeviceNotificationClient implementation
DeviceNotificationClient::DeviceNotificationClient(AudioWorker* worker)
    : m_cRef(1), m_worker(worker)
{
    qDebug() << "DeviceNotificationClient created";
}

DeviceNotificationClient::~DeviceNotificationClient()
{
    qDebug() << "DeviceNotificationClient destroyed";
}

HRESULT STDMETHODCALLTYPE DeviceNotificationClient::QueryInterface(REFIID riid, VOID **ppvInterface)
{
    if (IID_IUnknown == riid) {
        AddRef();
        *ppvInterface = (IUnknown*)this;
    } else if (__uuidof(IMMNotificationClient) == riid) {
        AddRef();
        *ppvInterface = (IMMNotificationClient*)this;
    } else {
        *ppvInterface = NULL;
        return E_NOINTERFACE;
    }
    return S_OK;
}

ULONG STDMETHODCALLTYPE DeviceNotificationClient::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

ULONG STDMETHODCALLTYPE DeviceNotificationClient::Release()
{
    ULONG ulRef = InterlockedDecrement(&m_cRef);
    if (0 == ulRef) {
        delete this;
    }
    return ulRef;
}

HRESULT STDMETHODCALLTYPE DeviceNotificationClient::OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState)
{
    QString deviceId = QString::fromWCharArray(pwstrDeviceId);
    qDebug() << "Device state changed:" << deviceId << "new state:" << dwNewState;

    if (m_worker) {
        // Re-enumerate devices to update states
        QMetaObject::invokeMethod(m_worker, "enumerateDevices", Qt::QueuedConnection);
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DeviceNotificationClient::OnDeviceAdded(LPCWSTR pwstrDeviceId)
{
    QString deviceId = QString::fromWCharArray(pwstrDeviceId);
    qDebug() << "Device added:" << deviceId;

    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "onDeviceAdded", Qt::QueuedConnection,
                                  Q_ARG(QString, deviceId));
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DeviceNotificationClient::OnDeviceRemoved(LPCWSTR pwstrDeviceId)
{
    QString deviceId = QString::fromWCharArray(pwstrDeviceId);
    qDebug() << "Device removed:" << deviceId;

    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "onDeviceRemoved", Qt::QueuedConnection,
                                  Q_ARG(QString, deviceId));
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DeviceNotificationClient::OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDefaultDeviceId)
{
    QString deviceId = pwstrDefaultDeviceId ? QString::fromWCharArray(pwstrDefaultDeviceId) : QString();
    AudioWorker::DataFlow dataFlow = AudioWorker::fromWindowsDataFlow(flow);

    qDebug() << "Default device changed:" << deviceId << "flow:" << flow << "role:" << role;

    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "onDefaultDeviceChanged", Qt::QueuedConnection,
                                  Q_ARG(AudioWorker::DataFlow, dataFlow),
                                  Q_ARG(QString, deviceId));
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DeviceNotificationClient::OnPropertyValueChanged(LPCWSTR pwstrDeviceId, const PROPERTYKEY key)
{
    // Re-enumerate devices to pick up property changes
    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "enumerateDevices", Qt::QueuedConnection);
    }
    return S_OK;
}

// VolumeNotificationClient implementation
VolumeNotificationClient::VolumeNotificationClient(AudioWorker* worker, EDataFlow dataFlow)
    : m_cRef(1), m_worker(worker), m_dataFlow(dataFlow)
{
    qDebug() << "VolumeNotificationClient created for dataFlow:" << dataFlow;
}

VolumeNotificationClient::~VolumeNotificationClient()
{
    qDebug() << "VolumeNotificationClient destroyed";
}

HRESULT STDMETHODCALLTYPE VolumeNotificationClient::QueryInterface(REFIID riid, VOID **ppvInterface)
{
    if (IID_IUnknown == riid) {
        AddRef();
        *ppvInterface = (IUnknown*)this;
    } else if (__uuidof(IAudioEndpointVolumeCallback) == riid) {
        AddRef();
        *ppvInterface = (IAudioEndpointVolumeCallback*)this;
    } else {
        *ppvInterface = NULL;
        return E_NOINTERFACE;
    }
    return S_OK;
}

ULONG STDMETHODCALLTYPE VolumeNotificationClient::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

ULONG STDMETHODCALLTYPE VolumeNotificationClient::Release()
{
    ULONG ulRef = InterlockedDecrement(&m_cRef);
    if (0 == ulRef) {
        delete this;
    }
    return ulRef;
}

HRESULT STDMETHODCALLTYPE VolumeNotificationClient::OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA pNotify)
{
    if (m_worker && pNotify) {
        float volume = pNotify->fMasterVolume;
        bool muted = pNotify->bMuted;

        AudioWorker::DataFlow dataFlow = AudioWorker::fromWindowsDataFlow(m_dataFlow);

        QMetaObject::invokeMethod(m_worker, "onVolumeChanged", Qt::QueuedConnection,
                                  Q_ARG(AudioWorker::DataFlow, dataFlow),
                                  Q_ARG(float, volume),
                                  Q_ARG(bool, muted));
    }
    return S_OK;
}

// SessionNotificationClient implementation
SessionNotificationClient::SessionNotificationClient(AudioWorker* worker)
    : m_cRef(1), m_worker(worker)
{
    qDebug() << "SessionNotificationClient created";
}

SessionNotificationClient::~SessionNotificationClient()
{
    qDebug() << "SessionNotificationClient destroyed";
}

HRESULT STDMETHODCALLTYPE SessionNotificationClient::QueryInterface(REFIID riid, VOID **ppvInterface)
{
    if (IID_IUnknown == riid) {
        AddRef();
        *ppvInterface = (IUnknown*)this;
    } else if (__uuidof(IAudioSessionNotification) == riid) {
        AddRef();
        *ppvInterface = (IAudioSessionNotification*)this;
    } else {
        *ppvInterface = NULL;
        return E_NOINTERFACE;
    }
    return S_OK;
}

ULONG STDMETHODCALLTYPE SessionNotificationClient::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

ULONG STDMETHODCALLTYPE SessionNotificationClient::Release()
{
    ULONG ulRef = InterlockedDecrement(&m_cRef);
    if (0 == ulRef) {
        delete this;
    }
    return ulRef;
}

HRESULT STDMETHODCALLTYPE SessionNotificationClient::OnSessionCreated(IAudioSessionControl *NewSession)
{
    if (m_worker) {
        qDebug() << "New audio session created";
        QMetaObject::invokeMethod(m_worker, "onSessionCreated", Qt::QueuedConnection);
    }
    return S_OK;
}

// SessionEventsClient implementation
SessionEventsClient::SessionEventsClient(AudioWorker* worker, const QString& appId)
    : m_cRef(1), m_worker(worker), m_appId(appId)
{
    qDebug() << "SessionEventsClient created for app:" << appId;
}

SessionEventsClient::~SessionEventsClient()
{
    qDebug() << "SessionEventsClient destroyed for app:" << m_appId;
}

HRESULT STDMETHODCALLTYPE SessionEventsClient::QueryInterface(REFIID riid, VOID **ppvInterface)
{
    if (IID_IUnknown == riid) {
        AddRef();
        *ppvInterface = (IUnknown*)this;
    } else if (__uuidof(IAudioSessionEvents) == riid) {
        AddRef();
        *ppvInterface = (IAudioSessionEvents*)this;
    } else {
        *ppvInterface = NULL;
        return E_NOINTERFACE;
    }
    return S_OK;
}

ULONG STDMETHODCALLTYPE SessionEventsClient::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

ULONG STDMETHODCALLTYPE SessionEventsClient::Release()
{
    ULONG ulRef = InterlockedDecrement(&m_cRef);
    if (0 == ulRef) {
        delete this;
    }
    return ulRef;
}

HRESULT STDMETHODCALLTYPE SessionEventsClient::OnSimpleVolumeChanged(float NewVolume, BOOL NewMute, LPCGUID EventContext)
{
    qDebug() << "*** SessionEventsClient::OnSimpleVolumeChanged ***";
    qDebug() << "App ID:" << m_appId;
    qDebug() << "New Volume:" << (NewVolume * 100.0f) << "%";
    qDebug() << "New Mute:" << (NewMute ? "true" : "false");
    qDebug() << "Thread ID:" << GetCurrentThreadId();

    if (m_worker) {
        bool success = QMetaObject::invokeMethod(m_worker, "onApplicationSessionVolumeChanged", Qt::QueuedConnection,
                                                 Q_ARG(QString, m_appId),
                                                 Q_ARG(float, NewVolume),
                                                 Q_ARG(bool, NewMute));
        qDebug() << "QMetaObject::invokeMethod success:" << success;
    } else {
        qDebug() << "ERROR: No worker available!";
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE SessionEventsClient::OnStateChanged(AudioSessionState NewState)
{
    if (NewState == AudioSessionStateExpired) {
        // Session ended, trigger re-enumeration
        if (m_worker) {
            QMetaObject::invokeMethod(m_worker, "onSessionDisconnected", Qt::QueuedConnection);
        }
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE SessionEventsClient::OnSessionDisconnected(AudioSessionDisconnectReason DisconnectReason)
{
    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "onSessionDisconnected", Qt::QueuedConnection);
    }
    return S_OK;
}

// Stub implementations for other required methods
HRESULT STDMETHODCALLTYPE SessionEventsClient::OnDisplayNameChanged(LPCWSTR NewDisplayName, LPCGUID EventContext) { return S_OK; }
HRESULT STDMETHODCALLTYPE SessionEventsClient::OnIconPathChanged(LPCWSTR NewIconPath, LPCGUID EventContext) { return S_OK; }
HRESULT STDMETHODCALLTYPE SessionEventsClient::OnChannelVolumeChanged(DWORD ChannelCount, float NewChannelVolumeArray[], DWORD ChangedChannel, LPCGUID EventContext) { return S_OK; }
HRESULT STDMETHODCALLTYPE SessionEventsClient::OnGroupingParamChanged(LPCGUID NewGroupingParam, LPCGUID EventContext) { return S_OK; }

// AudioWorker implementation
AudioWorker::AudioWorker()
    : m_deviceEnumerator(nullptr)
    , m_outputVolumeClient(nullptr)
    , m_inputVolumeClient(nullptr)
    , m_sessionNotificationClient(nullptr)
    , m_deviceNotificationClient(nullptr)
    , m_sessionManager(nullptr)
    , m_policyConfig(nullptr)
    , m_outputVolumeControl(nullptr)
    , m_inputVolumeControl(nullptr)
    , m_outputVolume(0)
    , m_inputVolume(0)
    , m_outputMuted(false)
    , m_inputMuted(false)
{
    qDebug() << "AudioWorker created on thread:" << QThread::currentThread();

    // Register the metatypes
    qRegisterMetaType<AudioApplication>("AudioApplication");
    qRegisterMetaType<QList<AudioApplication>>("QList<AudioApplication>");
    qRegisterMetaType<AudioDevice>("AudioDevice");
    qRegisterMetaType<QList<AudioDevice>>("QList<AudioDevice>");
}

AudioWorker::~AudioWorker()
{
    qDebug() << "AudioWorker destructor";
    cleanup();
}

void AudioWorker::initialize()
{
    qDebug() << "AudioWorker::initialize on thread:" << QThread::currentThread();

    HRESULT hr = initializeCOM();
    if (FAILED(hr)) {
        qDebug() << "Failed to initialize COM, HRESULT:" << QString::number(hr, 16);
        return;
    }
    qDebug() << "COM initialized successfully";

    // Create device enumerator
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                          __uuidof(IMMDeviceEnumerator), (void**)&m_deviceEnumerator);
    if (FAILED(hr)) {
        qDebug() << "Failed to create device enumerator, HRESULT:" << QString::number(hr, 16);
        return;
    }
    qDebug() << "Device enumerator created successfully";

    // Create policy config for setting default devices
    hr = CoCreateInstance(__uuidof(CPolicyConfigClient), nullptr, CLSCTX_ALL,
                          __uuidof(IPolicyConfig), (void**)&m_policyConfig);
    if (FAILED(hr)) {
        qDebug() << "Failed to create policy config, trying Vista version. HRESULT:" << QString::number(hr, 16);
        // Try Vista version as fallback
        hr = CoCreateInstance(__uuidof(CPolicyConfigVistaClient), nullptr, CLSCTX_ALL,
                              __uuidof(IPolicyConfigVista), (void**)&m_policyConfig);
        if (FAILED(hr)) {
            qDebug() << "Failed to create Vista policy config, HRESULT:" << QString::number(hr, 16);
            m_policyConfig = nullptr;
        }
    }

    // Set up notifications
    setupVolumeNotifications();
    setupSessionNotifications();
    setupDeviceNotifications();

    // Get initial state
    updateCurrentVolumes();
    enumerateDevices();
    enumerateApplications();

    qDebug() << "AudioWorker initialized successfully";
    emit initializationComplete();
}

void AudioWorker::cleanup()
{
    qDebug() << "AudioWorker::cleanup starting";

    // Clean up device notifications
    if (m_deviceEnumerator && m_deviceNotificationClient) {
        m_deviceEnumerator->UnregisterEndpointNotificationCallback(m_deviceNotificationClient);
    }
    if (m_deviceNotificationClient) {
        m_deviceNotificationClient->Release();
        m_deviceNotificationClient = nullptr;
    }

    // Clean up active sessions
    qDebug() << "Cleaning up" << m_activeSessions.count() << "active sessions";
    for (auto& sessionInfo : m_activeSessions) {
        if (sessionInfo.sessionControl && sessionInfo.eventsClient) {
            sessionInfo.sessionControl->UnregisterAudioSessionNotification(sessionInfo.eventsClient);
        }
        if (sessionInfo.eventsClient) {
            sessionInfo.eventsClient->Release();
        }
        if (sessionInfo.sessionControl) {
            sessionInfo.sessionControl->Release();
        }
    }
    m_activeSessions.clear();

    // Clean up session event clients (legacy)
    for (auto it = m_sessionEventClients.begin(); it != m_sessionEventClients.end(); ++it) {
        it.value()->Release();
    }
    m_sessionEventClients.clear();

    // Unregister session notifications
    if (m_sessionManager && m_sessionNotificationClient) {
        m_sessionManager->UnregisterSessionNotification(m_sessionNotificationClient);
        m_sessionManager->Release();
        m_sessionManager = nullptr;
    }

    if (m_sessionNotificationClient) {
        m_sessionNotificationClient->Release();
        m_sessionNotificationClient = nullptr;
    }

    // Cleanup volume notifications
    if (m_outputVolumeControl && m_outputVolumeClient) {
        m_outputVolumeControl->UnregisterControlChangeNotify(m_outputVolumeClient);
        m_outputVolumeControl->Release();
        m_outputVolumeControl = nullptr;
    }

    if (m_inputVolumeControl && m_inputVolumeClient) {
        m_inputVolumeControl->UnregisterControlChangeNotify(m_inputVolumeClient);
        m_inputVolumeControl->Release();
        m_inputVolumeControl = nullptr;
    }

    if (m_outputVolumeClient) {
        m_outputVolumeClient->Release();
        m_outputVolumeClient = nullptr;
    }

    if (m_inputVolumeClient) {
        m_inputVolumeClient->Release();
        m_inputVolumeClient = nullptr;
    }

    if (m_policyConfig) {
        m_policyConfig->Release();
        m_policyConfig = nullptr;
    }

    if (m_deviceEnumerator) {
        m_deviceEnumerator->Release();
        m_deviceEnumerator = nullptr;
    }

    CoUninitialize();
    qDebug() << "AudioWorker::cleanup completed";
}

HRESULT AudioWorker::initializeCOM()
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (hr == RPC_E_CHANGED_MODE) {
        CoUninitialize();
        hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    }
    return hr;
}

void AudioWorker::setupVolumeNotifications()
{
    if (!m_deviceEnumerator) return;

    // Setup output volume notifications
    CComPtr<IMMDevice> outputDevice;
    HRESULT hr = m_deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &outputDevice);
    if (SUCCEEDED(hr)) {
        hr = outputDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL,
                                    nullptr, (void**)&m_outputVolumeControl);
        if (SUCCEEDED(hr)) {
            m_outputVolumeClient = new VolumeNotificationClient(this, eRender);
            m_outputVolumeControl->RegisterControlChangeNotify(m_outputVolumeClient);
        }
    }

    // Setup input volume notifications
    CComPtr<IMMDevice> inputDevice;
    hr = m_deviceEnumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &inputDevice);
    if (SUCCEEDED(hr)) {
        hr = inputDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL,
                                   nullptr, (void**)&m_inputVolumeControl);
        if (SUCCEEDED(hr)) {
            m_inputVolumeClient = new VolumeNotificationClient(this, eCapture);
            m_inputVolumeControl->RegisterControlChangeNotify(m_inputVolumeClient);
        }
    }
}

void AudioWorker::setupSessionNotifications()
{
    if (!m_deviceEnumerator) return;

    CComPtr<IMMDevice> device;
    HRESULT hr = m_deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
    if (SUCCEEDED(hr)) {
        hr = device->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL,
                              nullptr, (void**)&m_sessionManager);
        if (SUCCEEDED(hr)) {
            m_sessionNotificationClient = new SessionNotificationClient(this);
            m_sessionManager->RegisterSessionNotification(m_sessionNotificationClient);
            qDebug() << "Session notifications registered successfully";
        }
    }
}

void AudioWorker::setupDeviceNotifications()
{
    if (!m_deviceEnumerator) return;

    m_deviceNotificationClient = new DeviceNotificationClient(this);
    HRESULT hr = m_deviceEnumerator->RegisterEndpointNotificationCallback(m_deviceNotificationClient);
    if (SUCCEEDED(hr)) {
        qDebug() << "Device notifications registered successfully";
    } else {
        qDebug() << "Failed to register device notifications, HRESULT:" << QString::number(hr, 16);
        m_deviceNotificationClient->Release();
        m_deviceNotificationClient = nullptr;
    }
}

void AudioWorker::updateCurrentVolumes()
{
    // Get current output volume and mute state
    if (m_outputVolumeControl) {
        float volume = 0.0f;
        BOOL muted = FALSE;

        if (SUCCEEDED(m_outputVolumeControl->GetMasterVolumeLevelScalar(&volume))) {
            m_outputVolume = static_cast<int>(std::round(volume * 100.0f));
            emit outputVolumeChanged(m_outputVolume);
        }

        if (SUCCEEDED(m_outputVolumeControl->GetMute(&muted))) {
            m_outputMuted = muted;
            emit outputMuteChanged(m_outputMuted);
        }
    }

    // Get current input volume and mute state
    if (m_inputVolumeControl) {
        float volume = 0.0f;
        BOOL muted = FALSE;

        if (SUCCEEDED(m_inputVolumeControl->GetMasterVolumeLevelScalar(&volume))) {
            m_inputVolume = static_cast<int>(std::round(volume * 100.0f));
            emit inputVolumeChanged(m_inputVolume);
        }

        if (SUCCEEDED(m_inputVolumeControl->GetMute(&muted))) {
            m_inputMuted = muted;
            emit inputMuteChanged(m_inputMuted);
        }
    }
}

void AudioWorker::enumerateDevices()
{
    if (!m_deviceEnumerator) {
        qDebug() << "No device enumerator available";
        return;
    }

    qDebug() << "=== ENUMERATING AUDIO DEVICES ===";

    QList<AudioDevice> newDevices;

    // Enumerate both input and output devices
    for (int dataFlowIndex = 0; dataFlowIndex < 2; ++dataFlowIndex) {
        EDataFlow dataFlow = (dataFlowIndex == 0) ? eRender : eCapture;
        QString flowName = (dataFlow == eRender) ? "Output" : "Input";

        qDebug() << "Enumerating" << flowName << "devices";

        CComPtr<IMMDeviceCollection> deviceCollection;
        HRESULT hr = m_deviceEnumerator->EnumAudioEndpoints(dataFlow, DEVICE_STATE_ACTIVE, &deviceCollection);
        if (FAILED(hr)) {
            qDebug() << "Failed to enumerate" << flowName << "devices, HRESULT:" << QString::number(hr, 16);
            continue;
        }

        UINT deviceCount = 0;
        deviceCollection->GetCount(&deviceCount);
        qDebug() << "Found" << deviceCount << flowName << "devices";

        // Get default devices for comparison
        CComPtr<IMMDevice> defaultDevice;
        CComPtr<IMMDevice> defaultCommDevice;
        m_deviceEnumerator->GetDefaultAudioEndpoint(dataFlow, eConsole, &defaultDevice);
        m_deviceEnumerator->GetDefaultAudioEndpoint(dataFlow, eCommunications, &defaultCommDevice);

        LPWSTR defaultDeviceId = nullptr;
        LPWSTR defaultCommDeviceId = nullptr;
        if (defaultDevice) {
            defaultDevice->GetId(&defaultDeviceId);
        }
        if (defaultCommDevice) {
            defaultCommDevice->GetId(&defaultCommDeviceId);
        }

        for (UINT i = 0; i < deviceCount; ++i) {
            CComPtr<IMMDevice> device;
            hr = deviceCollection->Item(i, &device);
            if (FAILED(hr)) continue;

            AudioDevice audioDevice = createAudioDeviceFromInterface(device, dataFlow);
            if (!audioDevice.id.isEmpty()) {
                // Check if this is the default device
                if (defaultDeviceId && audioDevice.id == QString::fromWCharArray(defaultDeviceId)) {
                    audioDevice.isDefault = true;
                }
                if (defaultCommDeviceId && audioDevice.id == QString::fromWCharArray(defaultCommDeviceId)) {
                    audioDevice.isDefaultCommunication = true;
                }

                newDevices.append(audioDevice);
                qDebug() << "  Device:" << audioDevice.name << "Default:" << audioDevice.isDefault << "DefaultComm:" << audioDevice.isDefaultCommunication;
            }
        }

        if (defaultDeviceId) {
            CoTaskMemFree(defaultDeviceId);
        }
        if (defaultCommDeviceId) {
            CoTaskMemFree(defaultCommDeviceId);
        }
    }

    // Update cached devices
    m_devices = newDevices;

    qDebug() << "=== DEVICE ENUMERATION COMPLETE ===";
    qDebug() << "Total devices found:" << newDevices.count();

    emit devicesChanged(newDevices);
}

AudioDevice AudioWorker::createAudioDeviceFromInterface(IMMDevice* device, EDataFlow dataFlow)
{
    AudioDevice audioDevice;

    if (!device) return audioDevice;

    // Get device ID
    LPWSTR deviceId = nullptr;
    HRESULT hr = device->GetId(&deviceId);
    if (SUCCEEDED(hr) && deviceId) {
        audioDevice.id = QString::fromWCharArray(deviceId);
        CoTaskMemFree(deviceId);
    }

    // Get device state
    DWORD state = 0;
    hr = device->GetState(&state);
    if (SUCCEEDED(hr)) {
        switch (state) {
        case DEVICE_STATE_ACTIVE:
            audioDevice.state = "Active";
            break;
        case DEVICE_STATE_DISABLED:
            audioDevice.state = "Disabled";
            break;
        case DEVICE_STATE_NOTPRESENT:
            audioDevice.state = "Not Present";
            break;
        case DEVICE_STATE_UNPLUGGED:
            audioDevice.state = "Unplugged";
            break;
        default:
            audioDevice.state = "Unknown";
            break;
        }
    }

    // Get device properties
    audioDevice.name = getDeviceProperty(device, PKEY_Device_FriendlyName);
    audioDevice.description = getDeviceProperty(device, PKEY_Device_DeviceDesc);
    audioDevice.isInput = (dataFlow == eCapture);

    if (audioDevice.name.isEmpty()) {
        audioDevice.name = audioDevice.description;
    }
    if (audioDevice.name.isEmpty()) {
        audioDevice.name = "Unknown Device";
    }

    return audioDevice;
}

QString AudioWorker::getDeviceProperty(IMMDevice* device, const PROPERTYKEY& key)
{
    if (!device) return QString();

    CComPtr<IPropertyStore> propertyStore;
    HRESULT hr = device->OpenPropertyStore(STGM_READ, &propertyStore);
    if (FAILED(hr)) return QString();

    PROPVARIANT propVariant;
    PropVariantInit(&propVariant);

    hr = propertyStore->GetValue(key, &propVariant);
    if (SUCCEEDED(hr) && propVariant.vt == VT_LPWSTR) {
        QString result = QString::fromWCharArray(propVariant.pwszVal);
        PropVariantClear(&propVariant);
        return result;
    }

    PropVariantClear(&propVariant);
    return QString();
}

void AudioWorker::enumerateApplications()
{
    if (!m_sessionManager) {
        qDebug() << "No session manager available";
        return;
    }

    qDebug() << "=== ENUMERATING APPLICATIONS ===";

    // Clean up existing sessions and event clients
    qDebug() << "Cleaning up" << m_activeSessions.count() << "existing sessions";
    for (auto& sessionInfo : m_activeSessions) {
        if (sessionInfo.sessionControl && sessionInfo.eventsClient) {
            sessionInfo.sessionControl->UnregisterAudioSessionNotification(sessionInfo.eventsClient);
        }
        if (sessionInfo.eventsClient) {
            sessionInfo.eventsClient->Release();
        }
        if (sessionInfo.sessionControl) {
            sessionInfo.sessionControl->Release();
        }
    }
    m_activeSessions.clear();

    // Also clean up the old map-based tracking
    for (auto it = m_sessionEventClients.begin(); it != m_sessionEventClients.end(); ++it) {
        it.value()->Release();
    }
    m_sessionEventClients.clear();

    QList<AudioApplication> newApplications;
    QMap<QString, AudioApplication> appMap; // To handle multiple sessions per app

    CComPtr<IAudioSessionEnumerator> sessionEnumerator;
    HRESULT hr = m_sessionManager->GetSessionEnumerator(&sessionEnumerator);
    if (FAILED(hr)) {
        qDebug() << "Failed to get session enumerator, HRESULT:" << QString::number(hr, 16);
        return;
    }

    int sessionCount = 0;
    sessionEnumerator->GetCount(&sessionCount);
    qDebug() << "Found" << sessionCount << "total sessions";

    for (int i = 0; i < sessionCount; ++i) {
        IAudioSessionControl* sessionControl = nullptr;
        hr = sessionEnumerator->GetSession(i, &sessionControl);
        if (FAILED(hr) || !sessionControl) {
            qDebug() << "Failed to get session" << i;
            continue;
        }

        CComPtr<IAudioSessionControl2> sessionControl2;
        hr = sessionControl->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&sessionControl2);
        if (FAILED(hr)) {
            qDebug() << "Failed to get session control 2 for session" << i;
            sessionControl->Release();
            continue;
        }

        DWORD processId = 0;
        hr = sessionControl2->GetProcessId(&processId);
        if (FAILED(hr)) {
            qDebug() << "Failed to get process ID for session" << i;
            sessionControl->Release();
            continue;
        }

        if (processId == 0 || processId == GetCurrentProcessId()) {
            qDebug() << "Skipping system or own process" << processId;
            sessionControl->Release();
            continue;
        }

        // Check if session is active
        AudioSessionState state;
        hr = sessionControl->GetState(&state);
        if (FAILED(hr) || state != AudioSessionStateActive) {
            qDebug() << "Session" << i << "for process" << processId << "is not active (state:" << state << ")";
            sessionControl->Release();
            continue;
        }

        QString appId = QString::number(processId);
        qDebug() << "Processing active session" << i << "for process ID" << processId;

        // Create or update application info
        AudioApplication app;
        if (appMap.contains(appId)) {
            app = appMap[appId];
        } else {
            app.id = appId;

            // Get process info
            QString execPath = getExecutablePath(processId);
            if (!execPath.isEmpty()) {
                QFileInfo fileInfo(execPath);
                app.executableName = fileInfo.baseName();
                app.name = getApplicationDisplayName(execPath);
                if (app.name.isEmpty()) {
                    app.name = app.executableName;
                }

                // Get icon
                QIcon icon = getApplicationIcon(execPath);
                if (!icon.isNull()) {
                    QPixmap pixmap = icon.pixmap(32, 32);
                    QByteArray byteArray;
                    QBuffer buffer(&byteArray);
                    buffer.open(QIODevice::WriteOnly);
                    pixmap.save(&buffer, "PNG");
                    app.iconPath = "data:image/png;base64," + byteArray.toBase64();
                }
            } else {
                app.name = QString("Process %1").arg(processId);
                app.executableName = app.name;
            }
        }

        // Get volume info from this session
        CComPtr<ISimpleAudioVolume> simpleVolume;
        hr = sessionControl->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&simpleVolume);
        if (SUCCEEDED(hr)) {
            float volume = 0.0f;
            BOOL muted = FALSE;
            if (SUCCEEDED(simpleVolume->GetMasterVolume(&volume))) {
                app.volume = static_cast<int>(std::round(volume * 100.0f));
            }
            if (SUCCEEDED(simpleVolume->GetMute(&muted))) {
                app.isMuted = muted;
            }
            qDebug() << "Session" << i << "- App" << processId << "Volume:" << app.volume << "% Muted:" << app.isMuted;
        }

        // Register session events for this session
        SessionEventsClient* eventsClient = new SessionEventsClient(this, appId);
        hr = sessionControl->RegisterAudioSessionNotification(eventsClient);
        if (SUCCEEDED(hr)) {
            // Keep the session control alive and track it
            SessionInfo sessionInfo;
            sessionInfo.sessionControl = sessionControl; // Don't release - we need to keep it alive
            sessionInfo.eventsClient = eventsClient;
            sessionInfo.appId = appId;
            m_activeSessions.append(sessionInfo);

            qDebug() << "✓ Successfully registered session events for app:" << appId << "(session" << i << ")";
        } else {
            eventsClient->Release();
            sessionControl->Release();
            qDebug() << "✗ Failed to register session events for app:" << appId << "HRESULT:" << QString::number(hr, 16);
        }

        appMap[appId] = app;
    }

    // Convert map to list
    for (auto it = appMap.begin(); it != appMap.end(); ++it) {
        newApplications.append(it.value());
    }

    // Update cached applications
    m_applications = newApplications;

    qDebug() << "=== ENUMERATION COMPLETE ===";
    qDebug() << "Total applications found:" << newApplications.count();
    qDebug() << "Active sessions tracked:" << m_activeSessions.count();

    emit applicationsChanged(newApplications);
}

QString AudioWorker::getExecutablePath(DWORD processId)
{
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (!hProcess) return QString();

    WCHAR path[MAX_PATH];
    DWORD pathSize = MAX_PATH;

    QString result;
    if (QueryFullProcessImageName(hProcess, 0, path, &pathSize)) {
        result = QString::fromWCharArray(path);
    }

    CloseHandle(hProcess);
    return result;
}

QIcon AudioWorker::getApplicationIcon(const QString& executablePath)
{
    SHFILEINFO shFileInfo;
    if (SHGetFileInfo(executablePath.toStdWString().c_str(),
                      0, &shFileInfo, sizeof(shFileInfo),
                      SHGFI_ICON | SHGFI_LARGEICON)) {

        // Convert HICON to QImage, then to QPixmap
        QImage image = QImage::fromHICON(shFileInfo.hIcon);
        QPixmap pixmap = QPixmap::fromImage(image);
        DestroyIcon(shFileInfo.hIcon);
        return QIcon(pixmap);
    }
    return QIcon();
}

QString AudioWorker::getApplicationDisplayName(const QString& executablePath)
{
    std::wstring wPath = executablePath.toStdWString();

    DWORD dwSize = GetFileVersionInfoSize(wPath.c_str(), NULL);
    if (dwSize == 0) return QString();

    std::vector<BYTE> buffer(dwSize);
    if (!GetFileVersionInfo(wPath.c_str(), 0, dwSize, buffer.data())) return QString();

    LPVOID lpBuffer = NULL;
    UINT uLen = 0;

    // Try ProductName first
    if (VerQueryValue(buffer.data(), L"\\StringFileInfo\\040904b0\\ProductName", &lpBuffer, &uLen) && uLen > 0) {
        return QString::fromWCharArray(static_cast<LPCWSTR>(lpBuffer));
    }

    // Try FileDescription
    if (VerQueryValue(buffer.data(), L"\\StringFileInfo\\040904b0\\FileDescription", &lpBuffer, &uLen) && uLen > 0) {
        return QString::fromWCharArray(static_cast<LPCWSTR>(lpBuffer));
    }

    return QString();
}

// Event handlers
void AudioWorker::onVolumeChanged(AudioWorker::DataFlow dataFlow, float volume, bool muted)
{
    int volumePercent = static_cast<int>(std::round(volume * 100.0f));

    if (dataFlow == Output) {
        if (m_outputVolume != volumePercent) {
            m_outputVolume = volumePercent;
            emit outputVolumeChanged(volumePercent);
        }
        if (m_outputMuted != muted) {
            m_outputMuted = muted;
            emit outputMuteChanged(muted);
        }
    } else if (dataFlow == Input) {
        if (m_inputVolume != volumePercent) {
            m_inputVolume = volumePercent;
            emit inputVolumeChanged(volumePercent);
        }
        if (m_inputMuted != muted) {
            m_inputMuted = muted;
            emit inputMuteChanged(muted);
        }
    }
}

void AudioWorker::onSessionCreated()
{
    qDebug() << "Session created, re-enumerating applications";
    enumerateApplications();
}

void AudioWorker::onSessionDisconnected()
{
    qDebug() << "Session disconnected, re-enumerating applications";
    enumerateApplications();
}

void AudioWorker::onDeviceAdded(const QString& deviceId)
{
    qDebug() << "Device added, re-enumerating devices";
    enumerateDevices();
}

void AudioWorker::onDeviceRemoved(const QString& deviceId)
{
    qDebug() << "Device removed, re-enumerating devices";
    enumerateDevices();
}

void AudioWorker::onDefaultDeviceChanged(DataFlow dataFlow, const QString& deviceId)
{
    qDebug() << "Default device changed, re-enumerating devices";
    enumerateDevices();

    // Also emit specific signal
    emit defaultDeviceChanged(deviceId, dataFlow == Input);
}

void AudioWorker::onApplicationSessionVolumeChanged(const QString& appId, float volume, bool muted)
{
    int volumePercent = static_cast<int>(std::round(volume * 100.0f));

    qDebug() << "=== APPLICATION SESSION VOLUME CHANGED ===";
    qDebug() << "App ID:" << appId;
    qDebug() << "Volume:" << volumePercent << "%";
    qDebug() << "Muted:" << muted;

    // Update our cached application list
    bool foundApp = false;
    for (int i = 0; i < m_applications.count(); ++i) {
        if (m_applications[i].id == appId) {
            foundApp = true;
            bool volumeChanged = (m_applications[i].volume != volumePercent);
            bool muteChanged = (m_applications[i].isMuted != muted);

            if (volumeChanged || muteChanged) {
                m_applications[i].volume = volumePercent;
                m_applications[i].isMuted = muted;

                qDebug() << "Updated app in cache - Volume changed:" << volumeChanged << "Mute changed:" << muteChanged;

                // Emit individual change signals
                if (volumeChanged) {
                    emit applicationVolumeChanged(appId, volumePercent);
                }
                if (muteChanged) {
                    emit applicationMuteChanged(appId, muted);
                }

                // Emit the full list to ensure model updates
                emit applicationsChanged(m_applications);
            }
            break;
        }
    }

    if (!foundApp) {
        qDebug() << "Warning: Application" << appId << "not found in cached list";
        // Re-enumerate to pick up any missed applications
        QMetaObject::invokeMethod(this, "enumerateApplications", Qt::QueuedConnection);
    }
}

// Control methods
void AudioWorker::setOutputVolume(int volume)
{
    setVolumeForDevice(eRender, volume);
}

void AudioWorker::setInputVolume(int volume)
{
    setVolumeForDevice(eCapture, volume);
}

void AudioWorker::setOutputMute(bool mute)
{
    setMuteForDevice(eRender, mute);
}

void AudioWorker::setInputMute(bool mute)
{
    setMuteForDevice(eCapture, mute);
}

void AudioWorker::setApplicationVolume(const QString& appId, int volume)
{
    if (!m_sessionManager) return;

    DWORD processId = appId.toULong();
    float volumeScalar = static_cast<float>(volume) / 100.0f;

    CComPtr<IAudioSessionEnumerator> sessionEnumerator;
    HRESULT hr = m_sessionManager->GetSessionEnumerator(&sessionEnumerator);
    if (FAILED(hr)) return;

    int sessionCount = 0;
    sessionEnumerator->GetCount(&sessionCount);

    for (int i = 0; i < sessionCount; ++i) {
        CComPtr<IAudioSessionControl> sessionControl;
        hr = sessionEnumerator->GetSession(i, &sessionControl);
        if (FAILED(hr)) continue;

        CComPtr<IAudioSessionControl2> sessionControl2;
        hr = sessionControl->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&sessionControl2);
        if (FAILED(hr)) continue;

        DWORD pid = 0;
        hr = sessionControl2->GetProcessId(&pid);
        if (FAILED(hr) || pid != processId) continue;

        CComPtr<ISimpleAudioVolume> simpleVolume;
        hr = sessionControl->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&simpleVolume);
        if (SUCCEEDED(hr)) {
            simpleVolume->SetMasterVolume(volumeScalar, nullptr);
            qDebug() << "Set application" << appId << "volume to" << volume;
        }
        break;
    }
}

void AudioWorker::setApplicationMute(const QString& appId, bool mute)
{
    if (!m_sessionManager) return;

    DWORD processId = appId.toULong();

    CComPtr<IAudioSessionEnumerator> sessionEnumerator;
    HRESULT hr = m_sessionManager->GetSessionEnumerator(&sessionEnumerator);
    if (FAILED(hr)) return;

    int sessionCount = 0;
    sessionEnumerator->GetCount(&sessionCount);

    for (int i = 0; i < sessionCount; ++i) {
        CComPtr<IAudioSessionControl> sessionControl;
        hr = sessionEnumerator->GetSession(i, &sessionControl);
        if (FAILED(hr)) continue;

        CComPtr<IAudioSessionControl2> sessionControl2;
        hr = sessionControl->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&sessionControl2);
        if (FAILED(hr)) continue;

        DWORD pid = 0;
        hr = sessionControl2->GetProcessId(&pid);
        if (FAILED(hr) || pid != processId) continue;

        CComPtr<ISimpleAudioVolume> simpleVolume;
        hr = sessionControl->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&simpleVolume);
        if (SUCCEEDED(hr)) {
            simpleVolume->SetMute(mute, nullptr);
            qDebug() << "Set application" << appId << "mute to" << mute;
        }
        break;
    }
}

void AudioWorker::setDefaultDevice(const QString& deviceId, bool isInput, bool forCommunications)
{
    if (!m_policyConfig) {
        qDebug() << "No policy config available, cannot set default device";
        return;
    }

    ERole role = forCommunications ? eCommunications : eConsole;
    std::wstring wDeviceId = deviceId.toStdWString();

    HRESULT hr = m_policyConfig->SetDefaultEndpoint(wDeviceId.c_str(), role);
    if (SUCCEEDED(hr)) {
        qDebug() << "Successfully set default device:" << deviceId << "isInput:" << isInput << "forCommunications:" << forCommunications;
    } else {
        qDebug() << "Failed to set default device, HRESULT:" << QString::number(hr, 16);
    }
}

void AudioWorker::setVolumeForDevice(EDataFlow dataFlow, int volume)
{
    IAudioEndpointVolume* volumeControl = (dataFlow == eRender) ? m_outputVolumeControl : m_inputVolumeControl;
    if (volumeControl) {
        float volumeScalar = static_cast<float>(volume) / 100.0f;
        volumeControl->SetMasterVolumeLevelScalar(volumeScalar, nullptr);
    }
}

void AudioWorker::setMuteForDevice(EDataFlow dataFlow, bool mute)
{
    IAudioEndpointVolume* volumeControl = (dataFlow == eRender) ? m_outputVolumeControl : m_inputVolumeControl;
    if (volumeControl) {
        volumeControl->SetMute(mute, nullptr);
    }
}

// AudioManager implementation
AudioManager::AudioManager(QObject* parent)
    : QObject(parent)
    , m_workerThread(nullptr)
    , m_worker(nullptr)
    , m_cachedOutputVolume(0)
    , m_cachedInputVolume(0)
    , m_cachedOutputMute(false)
    , m_cachedInputMute(false)
{
    qDebug() << "AudioManager created";
}

AudioManager::~AudioManager()
{
    qDebug() << "AudioManager destructor";
    cleanup();
}

AudioManager* AudioManager::instance()
{
    QMutexLocker locker(&m_mutex);
    if (!m_instance) {
        m_instance = new AudioManager();
    }
    return m_instance;
}

void AudioManager::initialize()
{
    QMutexLocker locker(&m_mutex);

    if (m_workerThread) {
        qDebug() << "AudioManager already initialized";
        return;
    }

    qDebug() << "AudioManager::initialize";

    m_workerThread = new QThread(this);
    m_worker = new AudioWorker();

    // Connect signals
    connect(m_worker, &AudioWorker::outputVolumeChanged, this, [this](int volume) {
        QMutexLocker cacheLock(&m_cacheMutex);
        m_cachedOutputVolume = volume;
        emit outputVolumeChanged(volume);
    }, Qt::QueuedConnection);

    connect(m_worker, &AudioWorker::inputVolumeChanged, this, [this](int volume) {
        QMutexLocker cacheLock(&m_cacheMutex);
        m_cachedInputVolume = volume;
        emit inputVolumeChanged(volume);
    }, Qt::QueuedConnection);

    connect(m_worker, &AudioWorker::outputMuteChanged, this, [this](bool muted) {
        QMutexLocker cacheLock(&m_cacheMutex);
        m_cachedOutputMute = muted;
        emit outputMuteChanged(muted);
    }, Qt::QueuedConnection);

    connect(m_worker, &AudioWorker::inputMuteChanged, this, [this](bool muted) {
        QMutexLocker cacheLock(&m_cacheMutex);
        m_cachedInputMute = muted;
        emit inputMuteChanged(muted);
    }, Qt::QueuedConnection);

    connect(m_worker, &AudioWorker::applicationsChanged, this, [this](const QList<AudioApplication>& apps) {
        QMutexLocker cacheLock(&m_cacheMutex);
        m_cachedApplications = apps;
        emit applicationsChanged(apps);
    }, Qt::QueuedConnection);

    connect(m_worker, &AudioWorker::devicesChanged, this, [this](const QList<AudioDevice>& devices) {
        QMutexLocker cacheLock(&m_cacheMutex);
        m_cachedDevices = devices;
        emit devicesChanged(devices);
    }, Qt::QueuedConnection);

    connect(m_worker, &AudioWorker::applicationVolumeChanged, this, &AudioManager::applicationVolumeChanged, Qt::QueuedConnection);
    connect(m_worker, &AudioWorker::applicationMuteChanged, this, &AudioManager::applicationMuteChanged, Qt::QueuedConnection);
    connect(m_worker, &AudioWorker::deviceAdded, this, &AudioManager::deviceAdded, Qt::QueuedConnection);
    connect(m_worker, &AudioWorker::deviceRemoved, this, &AudioManager::deviceRemoved, Qt::QueuedConnection);
    connect(m_worker, &AudioWorker::defaultDeviceChanged, this, &AudioManager::defaultDeviceChanged, Qt::QueuedConnection);
    connect(m_worker, &AudioWorker::initializationComplete, this, &AudioManager::initializationComplete, Qt::QueuedConnection);

    m_worker->moveToThread(m_workerThread);
    connect(m_workerThread, &QThread::started, m_worker, &AudioWorker::initialize);
    m_workerThread->start();

    qDebug() << "AudioManager initialization started";
}

void AudioManager::cleanup()
{
    QMutexLocker locker(&m_mutex);

    if (!m_workerThread) return;

    qDebug() << "AudioManager::cleanup";

    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "cleanup", Qt::QueuedConnection);
    }

    m_workerThread->quit();
    if (!m_workerThread->wait(5000)) {
        m_workerThread->terminate();
        m_workerThread->wait();
    }

    delete m_worker;
    m_worker = nullptr;

    delete m_workerThread;
    m_workerThread = nullptr;

    QMutexLocker cacheLock(&m_cacheMutex);
    m_cachedOutputVolume = 0;
    m_cachedInputVolume = 0;
    m_cachedOutputMute = false;
    m_cachedInputMute = false;
    m_cachedApplications.clear();
    m_cachedDevices.clear();
}

// Async methods
void AudioManager::setOutputVolumeAsync(int volume)
{
    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "setOutputVolume", Qt::QueuedConnection, Q_ARG(int, volume));
    }
}

void AudioManager::setInputVolumeAsync(int volume)
{
    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "setInputVolume", Qt::QueuedConnection, Q_ARG(int, volume));
    }
}

void AudioManager::setOutputMuteAsync(bool mute)
{
    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "setOutputMute", Qt::QueuedConnection, Q_ARG(bool, mute));
    }
}

void AudioManager::setInputMuteAsync(bool mute)
{
    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "setInputMute", Qt::QueuedConnection, Q_ARG(bool, mute));
    }
}

void AudioManager::setApplicationVolumeAsync(const QString& appId, int volume)
{
    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "setApplicationVolume", Qt::QueuedConnection,
                                  Q_ARG(QString, appId), Q_ARG(int, volume));
    }
}

void AudioManager::setApplicationMuteAsync(const QString& appId, bool mute)
{
    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "setApplicationMute", Qt::QueuedConnection,
                                  Q_ARG(QString, appId), Q_ARG(bool, mute));
    }
}

void AudioManager::setDefaultDeviceAsync(const QString& deviceId, bool isInput, bool forCommunications)
{
    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "setDefaultDevice", Qt::QueuedConnection,
                                  Q_ARG(QString, deviceId), Q_ARG(bool, isInput), Q_ARG(bool, forCommunications));
    }
}

// Cached getters
int AudioManager::getOutputVolume() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cachedOutputVolume;
}

int AudioManager::getInputVolume() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cachedInputVolume;
}

bool AudioManager::getOutputMute() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cachedOutputMute;
}

bool AudioManager::getInputMute() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cachedInputMute;
}

QList<AudioApplication> AudioManager::getApplications() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cachedApplications;
}

QList<AudioDevice> AudioManager::getDevices() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cachedDevices;
}

AudioWorker* AudioManager::getWorker()
{
    return m_worker;
}

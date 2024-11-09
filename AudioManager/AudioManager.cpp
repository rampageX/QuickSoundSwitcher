#include "AudioManager.h"
#include "PolicyConfig.h"
#include <atlbase.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <QDebug>

void AudioManager::initialize() {
    CoInitialize(nullptr);
}

void AudioManager::cleanup() {
    CoUninitialize();
}

QString extractShortName(const QString& fullName) {
    int firstOpenParenIndex = fullName.indexOf('(');
    int lastCloseParenIndex = fullName.lastIndexOf(')');

    if (firstOpenParenIndex != -1 && lastCloseParenIndex != -1 && firstOpenParenIndex < lastCloseParenIndex) {
        return fullName.mid(firstOpenParenIndex + 1, lastCloseParenIndex - firstOpenParenIndex - 1).trimmed();
    }

    return fullName;
}

void enumerateDevices(EDataFlow dataFlow, QList<AudioDevice>& devices) {
    CComPtr<IMMDeviceEnumerator> pEnumerator;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);

    if (FAILED(hr)) {
        qDebug() << "Failed to create device enumerator";
        return;
    }

    CComPtr<IMMDeviceCollection> pCollection;
    hr = pEnumerator->EnumAudioEndpoints(dataFlow, DEVICE_STATE_ACTIVE, &pCollection);
    if (FAILED(hr)) {
        qDebug() << "Failed to enumerate devices";
        return;
    }

    // Get the default audio endpoint for this data flow and role
    CComPtr<IMMDevice> pDefaultDevice;
    hr = pEnumerator->GetDefaultAudioEndpoint(dataFlow, eConsole, &pDefaultDevice);
    if (FAILED(hr)) {
        qDebug() << "Failed to get default audio endpoint";
        return;
    }

    LPWSTR pwszDefaultID = nullptr;
    hr = pDefaultDevice->GetId(&pwszDefaultID);
    if (FAILED(hr)) {
        qDebug() << "Failed to get default device ID";
        return;
    }

    UINT count;
    pCollection->GetCount(&count);

    for (UINT i = 0; i < count; ++i) {
        CComPtr<IMMDevice> pDevice;
        pCollection->Item(i, &pDevice);

        LPWSTR pwszID = nullptr;
        pDevice->GetId(&pwszID);

        CComPtr<IPropertyStore> pProps;
        pDevice->OpenPropertyStore(STGM_READ, &pProps);

        PROPVARIANT varName;
        PropVariantInit(&varName);
        pProps->GetValue(PKEY_Device_FriendlyName, &varName);

        AudioDevice device;
        device.id = QString::fromWCharArray(pwszID);
        device.name = QString::fromWCharArray(varName.pwszVal);
        device.shortName = extractShortName(device.name); // Customize if needed
        device.type = (dataFlow == eRender) ? "Playback" : "Recording";
        device.isDefault = (wcscmp(pwszID, pwszDefaultID) == 0); // Check if this device is the default one

        devices.append(device);

        CoTaskMemFree(pwszID);
        PropVariantClear(&varName);
    }

    CoTaskMemFree(pwszDefaultID);
}

void AudioManager::enumeratePlaybackDevices(QList<AudioDevice>& playbackDevices) {
    enumerateDevices(eRender, playbackDevices);
}

void AudioManager::enumerateRecordingDevices(QList<AudioDevice>& recordingDevices) {
    enumerateDevices(eCapture, recordingDevices);
}

void setVolume(EDataFlow dataFlow, int volume) {
    CComPtr<IMMDeviceEnumerator> pEnumerator;
    CComPtr<IMMDevice> pDevice;
    CComPtr<IAudioEndpointVolume> pVolumeControl;

    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (SUCCEEDED(hr)) {
        hr = pEnumerator->GetDefaultAudioEndpoint(dataFlow, eConsole, &pDevice);
    }
    if (SUCCEEDED(hr)) {
        hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, (void**)&pVolumeControl);
    }
    if (SUCCEEDED(hr)) {
        pVolumeControl->SetMasterVolumeLevelScalar(static_cast<float>(volume) / 100.0f, nullptr);
    }
}

int getVolume(EDataFlow dataFlow) {
    CComPtr<IMMDeviceEnumerator> pEnumerator;
    CComPtr<IMMDevice> pDevice;
    CComPtr<IAudioEndpointVolume> pVolumeControl;
    float volume = 0.0f;

    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (SUCCEEDED(hr)) {
        hr = pEnumerator->GetDefaultAudioEndpoint(dataFlow, eConsole, &pDevice);
    }
    if (SUCCEEDED(hr)) {
        hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, (void**)&pVolumeControl);
    }
    if (SUCCEEDED(hr)) {
        pVolumeControl->GetMasterVolumeLevelScalar(&volume);
    }
    return static_cast<int>(volume * 100.0f);
}

void setMute(EDataFlow dataFlow, bool mute) {
    CComPtr<IMMDeviceEnumerator> pEnumerator;
    CComPtr<IMMDevice> pDevice;
    CComPtr<IAudioEndpointVolume> pVolumeControl;

    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (SUCCEEDED(hr)) {
        hr = pEnumerator->GetDefaultAudioEndpoint(dataFlow, eConsole, &pDevice);
    }
    if (SUCCEEDED(hr)) {
        hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, (void**)&pVolumeControl);
    }
    if (SUCCEEDED(hr)) {
        pVolumeControl->SetMute(mute, nullptr);
    }
}

bool getMute(EDataFlow dataFlow) {
    CComPtr<IMMDeviceEnumerator> pEnumerator;
    CComPtr<IMMDevice> pDevice;
    CComPtr<IAudioEndpointVolume> pVolumeControl;
    BOOL mute = FALSE;

    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (SUCCEEDED(hr)) {
        hr = pEnumerator->GetDefaultAudioEndpoint(dataFlow, eConsole, &pDevice);
    }
    if (SUCCEEDED(hr)) {
        hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, (void**)&pVolumeControl);
    }
    if (SUCCEEDED(hr)) {
        pVolumeControl->GetMute(&mute);
    }
    return mute;
}

void AudioManager::setPlaybackVolume(int volume) {
    setVolume(eRender, volume);
}

int AudioManager::getPlaybackVolume() {
    return getVolume(eRender);
}

void AudioManager::setRecordingVolume(int volume) {
    setVolume(eCapture, volume);
}

int AudioManager::getRecordingVolume() {
    return getVolume(eCapture);
}

void AudioManager::setPlaybackMute(bool mute) {
    setMute(eRender, mute);
}

bool AudioManager::getPlaybackMute() {
    return getMute(eRender);
}

void AudioManager::setRecordingMute(bool mute) {
    setMute(eCapture, mute);
}

bool AudioManager::getRecordingMute() {
    return getMute(eCapture);
}

int getAudioLevelPercentage(EDataFlow dataFlow) {
    CComPtr<IMMDeviceEnumerator> pEnumerator;
    CComPtr<IMMDevice> pDevice;
    CComPtr<IAudioMeterInformation> pMeterInfo;
    float peakLevel = 0.0f;

    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (SUCCEEDED(hr)) {
        hr = pEnumerator->GetDefaultAudioEndpoint(dataFlow, eConsole, &pDevice);
    }
    if (SUCCEEDED(hr)) {
        hr = pDevice->Activate(__uuidof(IAudioMeterInformation), CLSCTX_ALL, nullptr, (void**)&pMeterInfo);
    }
    if (SUCCEEDED(hr)) {
        hr = pMeterInfo->GetPeakValue(&peakLevel);
        if (FAILED(hr)) {
            qDebug() << "Failed to get audio peak level";
            return 0;
        }
    } else {
        qDebug() << "Failed to activate IAudioMeterInformation";
        return 0;
    }

    return static_cast<int>(peakLevel * 100);
}

int AudioManager::getPlaybackAudioLevel() {
    return getAudioLevelPercentage(eRender);
}

int AudioManager::getRecordingAudioLevel() {
    return getAudioLevelPercentage(eCapture);
}

bool AudioManager::setDefaultEndpoint(const QString &deviceId)
{
    HRESULT hr;
    IMMDeviceEnumerator *deviceEnumerator = nullptr;
    IMMDevice *defaultDevice = nullptr;
    IPolicyConfig *policyConfig = nullptr;
    IPolicyConfigVista *policyConfigVista = nullptr;

    // Initialize COM library
    hr = CoInitialize(nullptr);
    if (FAILED(hr)) {
        qDebug() << "Failed to initialize COM library";
        return false;
    }

    // Create device enumerator
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER,
                          IID_PPV_ARGS(&deviceEnumerator));
    if (FAILED(hr)) {
        qDebug() << "Failed to create device enumerator";
        CoUninitialize();
        return false;
    }

    // Get the device by ID
    hr = deviceEnumerator->GetDevice(reinterpret_cast<LPCWSTR>(deviceId.utf16()), &defaultDevice);
    if (FAILED(hr)) {
        qDebug() << "Failed to get device by ID";
        deviceEnumerator->Release();
        CoUninitialize();
        return false;
    }

    // Attempt to get IPolicyConfig interface
    hr = CoCreateInstance(__uuidof(CPolicyConfigClient), nullptr, CLSCTX_ALL,
                          IID_PPV_ARGS(&policyConfig));
    if (FAILED(hr)) {
        // If IPolicyConfig is unavailable, try IPolicyConfigVista
        hr = CoCreateInstance(__uuidof(CPolicyConfigVistaClient), nullptr, CLSCTX_ALL,
                              IID_PPV_ARGS(&policyConfigVista));
        if (FAILED(hr)) {
            qDebug() << "Failed to create PolicyConfig interface";
            defaultDevice->Release();
            deviceEnumerator->Release();
            CoUninitialize();
            return false;
        }
    }

    // Set as default device for all audio roles (Console, Multimedia, Communications)
    if (policyConfig) {
        hr = policyConfig->SetDefaultEndpoint(reinterpret_cast<LPCWSTR>(deviceId.utf16()), eConsole);
        policyConfig->SetDefaultEndpoint(reinterpret_cast<LPCWSTR>(deviceId.utf16()), eMultimedia);
        policyConfig->SetDefaultEndpoint(reinterpret_cast<LPCWSTR>(deviceId.utf16()), eCommunications);
    } else if (policyConfigVista) {
        hr = policyConfigVista->SetDefaultEndpoint(reinterpret_cast<LPCWSTR>(deviceId.utf16()), eConsole);
        policyConfigVista->SetDefaultEndpoint(reinterpret_cast<LPCWSTR>(deviceId.utf16()), eMultimedia);
        policyConfigVista->SetDefaultEndpoint(reinterpret_cast<LPCWSTR>(deviceId.utf16()), eCommunications);
    }

    // Release resources
    if (policyConfig) policyConfig->Release();
    if (policyConfigVista) policyConfigVista->Release();
    defaultDevice->Release();
    deviceEnumerator->Release();
    CoUninitialize();

    if (FAILED(hr)) {
        qDebug() << "Failed to set default audio output device";
        return false;
    }

    qDebug() << "Default audio output device set to:" << deviceId;
    return true;
}


#include "AudioManager.h"
#include <atlbase.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <QDebug>

namespace AudioManager {

void initialize() {
    CoInitialize(nullptr);
}

void cleanup() {
    CoUninitialize();
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

QString extractShortName(const QString& fullName) {
    int firstOpenParenIndex = fullName.indexOf('(');
    int lastCloseParenIndex = fullName.lastIndexOf(')');

    if (firstOpenParenIndex != -1 && lastCloseParenIndex != -1 && firstOpenParenIndex < lastCloseParenIndex) {
        return fullName.mid(firstOpenParenIndex + 1, lastCloseParenIndex - firstOpenParenIndex - 1).trimmed();
    }

    return fullName;
}

void enumeratePlaybackDevices(QList<AudioDevice>& playbackDevices) {
    enumerateDevices(eRender, playbackDevices);
}

void enumerateRecordingDevices(QList<AudioDevice>& recordingDevices) {
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

void setPlaybackVolume(int volume) {
    setVolume(eRender, volume);
}

int getPlaybackVolume() {
    return getVolume(eRender);
}

void setRecordingVolume(int volume) {
    setVolume(eCapture, volume);
}

int getRecordingVolume() {
    return getVolume(eCapture);
}

void setPlaybackMute(bool mute) {
    setMute(eRender, mute);
}

bool getPlaybackMute() {
    return getMute(eRender);
}

void setRecordingMute(bool mute) {
    setMute(eCapture, mute);
}

bool getRecordingMute() {
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

int getPlaybackAudioLevel() {
    return getAudioLevelPercentage(eRender);
}

int getRecordingAudioLevel() {
    return getAudioLevelPercentage(eCapture);
}

}

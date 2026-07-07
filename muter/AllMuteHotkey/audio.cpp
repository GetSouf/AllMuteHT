#include "audio.h"

#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <propsys.h>
#include <propkey.h>
#include <functiondiscoverykeys_devpkey.h>

namespace amh {

namespace {

struct ComInit {
    ComInit() { CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED); }
    ~ComInit() { CoUninitialize(); }
};

bool set_device_mute(IMMDevice* device, bool muted) {
    IAudioEndpointVolume* volume = nullptr;
    if (FAILED(device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr,
                                reinterpret_cast<void**>(&volume)))) {
        return false;
    }

    const HRESULT hr = volume->SetMute(muted ? TRUE : FALSE, nullptr);
    volume->Release();
    return SUCCEEDED(hr);
}

bool get_device_mute(IMMDevice* device, bool& muted) {
    IAudioEndpointVolume* volume = nullptr;
    if (FAILED(device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr,
                                reinterpret_cast<void**>(&volume)))) {
        return false;
    }

    BOOL state = FALSE;
    const HRESULT hr = volume->GetMute(&state);
    volume->Release();
    if (FAILED(hr)) {
        return false;
    }

    muted = state == TRUE;
    return true;
}

std::wstring get_device_name(IMMDevice* device) {
    IPropertyStore* store = nullptr;
    if (FAILED(device->OpenPropertyStore(STGM_READ, &store))) {
        return L"(unknown)";
    }

    PROPVARIANT value;
    PropVariantInit(&value);
    std::wstring name = L"(unknown)";
    if (SUCCEEDED(store->GetValue(PKEY_Device_FriendlyName, &value)) && value.vt == VT_LPWSTR) {
        name = value.pwszVal;
    }

    PropVariantClear(&value);
    store->Release();
    return name;
}

std::wstring get_device_id(IMMDevice* device) {
    LPWSTR id = nullptr;
    if (FAILED(device->GetId(&id))) {
        return {};
    }
    std::wstring result = id;
    CoTaskMemFree(id);
    return result;
}

IMMDeviceEnumerator* create_enumerator() {
    IMMDeviceEnumerator* enumerator = nullptr;
    if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                __uuidof(IMMDeviceEnumerator),
                                reinterpret_cast<void**>(&enumerator)))) {
        return nullptr;
    }
    return enumerator;
}

}  // namespace

std::vector<MicrophoneState> list_microphones() {
    ComInit com;
    std::vector<MicrophoneState> result;

    IMMDeviceEnumerator* enumerator = create_enumerator();
    if (!enumerator) {
        return result;
    }

    IMMDeviceCollection* collection = nullptr;
    if (FAILED(enumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &collection))) {
        enumerator->Release();
        return result;
    }

    UINT count = 0;
    collection->GetCount(&count);
    for (UINT i = 0; i < count; ++i) {
        IMMDevice* device = nullptr;
        if (FAILED(collection->Item(i, &device))) {
            continue;
        }

        MicrophoneState mic;
        mic.id = get_device_id(device);
        mic.name = get_device_name(device);
        mic.reachable = get_device_mute(device, mic.muted);
        result.push_back(std::move(mic));
        device->Release();
    }

    collection->Release();
    enumerator->Release();
    return result;
}

bool set_all_microphones_muted(bool muted) {
    ComInit com;
    const auto mics = list_microphones();
    if (mics.empty()) {
        return false;
    }

    IMMDeviceEnumerator* enumerator = create_enumerator();
    if (!enumerator) {
        return false;
    }

    bool any_success = false;
    IMMDeviceCollection* collection = nullptr;
    if (FAILED(enumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &collection))) {
        enumerator->Release();
        return false;
    }

    UINT count = 0;
    collection->GetCount(&count);
    for (UINT i = 0; i < count; ++i) {
        IMMDevice* device = nullptr;
        if (FAILED(collection->Item(i, &device))) {
            continue;
        }
        any_success = set_device_mute(device, muted) || any_success;
        device->Release();
    }

    collection->Release();
    enumerator->Release();
    return any_success;
}

bool are_all_microphones_muted() {
    const auto mics = list_microphones();
    if (mics.empty()) {
        return false;
    }

    for (const auto& mic : mics) {
        if (!mic.reachable || !mic.muted) {
            return false;
        }
    }
    return true;
}

bool toggle_all_microphones(bool& now_muted) {
    const bool target = !are_all_microphones_muted();
    if (!set_all_microphones_muted(target)) {
        return false;
    }
    now_muted = target;
    return true;
}

}  // namespace amh

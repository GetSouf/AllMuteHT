#include "daemon.h"

#include "audio.h"
#include "hotkey.h"

#include <windows.h>

namespace amh {

namespace {

struct DaemonState {
    Config config;
    HANDLE stop_event = nullptr;
};

DaemonState* g_state = nullptr;

void play_notify_sound(bool muted) {
    MessageBeep(muted ? MB_ICONASTERISK : MB_ICONHAND);
}

LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
        case WM_HOTKEY:
            if (wparam == kHotkeyId && g_state != nullptr) {
                bool now_muted = false;
                if (toggle_all_microphones(now_muted) && g_state->config.notify_sound) {
                    play_notify_sound(now_muted);
                }
            }
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcW(hwnd, message, wparam, lparam);
    }
}

bool register_window_class() {
    WNDCLASSW wc = {};
    wc.lpfnWndProc = window_proc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = kWindowClassName;
    return RegisterClassW(&wc) != 0 || GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

HWND create_message_window() {
    return CreateWindowExW(0, kWindowClassName, L"AllMuteHotkey", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr,
                           GetModuleHandleW(nullptr), nullptr);
}

}  // namespace

bool is_daemon_running() {
    HANDLE mutex = OpenMutexW(SYNCHRONIZE, FALSE, kMutexName);
    if (!mutex) {
        return false;
    }
    CloseHandle(mutex);
    return true;
}

bool signal_daemon_stop() {
    HANDLE event = OpenEventW(EVENT_MODIFY_STATE, FALSE, kStopEventName);
    if (!event) {
        return false;
    }
    const BOOL ok = SetEvent(event);
    CloseHandle(event);
    return ok == TRUE;
}

int run_daemon(const Config& config) {
    HANDLE mutex = CreateMutexW(nullptr, TRUE, kMutexName);
    if (!mutex) {
        return 1;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(mutex);
        return 2;
    }

    HANDLE stop_event = CreateEventW(nullptr, TRUE, FALSE, kStopEventName);
    if (!stop_event) {
        CloseHandle(mutex);
        return 1;
    }

    if (HWND console = GetConsoleWindow()) {
        ShowWindow(console, SW_HIDE);
    }

    DaemonState state{config, stop_event};
    g_state = &state;

    if (!register_window_class()) {
        CloseHandle(stop_event);
        ReleaseMutex(mutex);
        CloseHandle(mutex);
        return 1;
    }

    HWND hwnd = create_message_window();
    if (!hwnd) {
        CloseHandle(stop_event);
        ReleaseMutex(mutex);
        CloseHandle(mutex);
        return 1;
    }

    if (!RegisterHotKey(hwnd, kHotkeyId, config.hotkey_modifiers, config.hotkey_vk)) {
        DestroyWindow(hwnd);
        CloseHandle(stop_event);
        ReleaseMutex(mutex);
        CloseHandle(mutex);
        return 3;
    }

    bool running = true;
    while (running) {
        const DWORD wait_result =
            MsgWaitForMultipleObjects(1, &stop_event, FALSE, INFINITE, QS_ALLINPUT);
        if (wait_result == WAIT_OBJECT_0) {
            break;
        }

        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    UnregisterHotKey(hwnd, kHotkeyId);
    DestroyWindow(hwnd);
    CloseHandle(stop_event);
    ReleaseMutex(mutex);
    CloseHandle(mutex);
    g_state = nullptr;
    return 0;
}

}  // namespace amh

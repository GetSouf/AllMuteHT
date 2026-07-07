#pragma once

#include "config.h"

namespace amh {

constexpr wchar_t kMutexName[] = L"Global\\AllMuteHotkey_Daemon";
constexpr wchar_t kStopEventName[] = L"Global\\AllMuteHotkey_StopEvent";
constexpr wchar_t kWindowClassName[] = L"AllMuteHotkeyMessageWindow";
constexpr int kHotkeyId = 1;

bool is_daemon_running();
bool signal_daemon_stop();
int run_daemon(const Config& config);

}  // namespace amh

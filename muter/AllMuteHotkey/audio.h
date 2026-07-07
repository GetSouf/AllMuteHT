#pragma once

#include <string>
#include <vector>

namespace amh {

struct MicrophoneState {
    std::wstring id;
    std::wstring name;
    bool muted = false;
    bool reachable = true;
};

std::vector<MicrophoneState> list_microphones();
bool set_all_microphones_muted(bool muted);
bool toggle_all_microphones(bool& now_muted);
bool are_all_microphones_muted();

}  // namespace amh

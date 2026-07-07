#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

namespace amh {

struct Config {
    std::uint32_t hotkey_modifiers = 0;
    std::uint32_t hotkey_vk = 0;
    bool autostart = false;
    bool notify_sound = true;
    bool configured = false;
    std::string language = "ru";
};

std::filesystem::path config_directory();
std::filesystem::path config_file_path();

std::optional<Config> load_config();
bool save_config(const Config& config);

}  // namespace amh

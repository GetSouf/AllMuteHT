#include "config.h"

#include <fstream>
#include <sstream>

#include <ShlObj.h>
#include <windows.h>

namespace amh {

namespace {

std::wstring to_wide(const std::string& text) {
    if (text.empty()) {
        return {};
    }
    const int size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (size <= 0) {
        return {};
    }
    std::wstring wide(static_cast<std::size_t>(size - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, wide.data(), size);
    return wide;
}

std::string trim(const std::string& text) {
    const auto start = text.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return {};
    }
    const auto end = text.find_last_not_of(" \t\r\n");
    return text.substr(start, end - start + 1);
}

bool parse_bool(const std::string& value, bool& out) {
    if (value == "1" || value == "true" || value == "yes") {
        out = true;
        return true;
    }
    if (value == "0" || value == "false" || value == "no") {
        out = false;
        return true;
    }
    return false;
}

}  // namespace

std::filesystem::path config_directory() {
    PWSTR path = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &path))) {
        return {};
    }
    std::filesystem::path result = path;
    CoTaskMemFree(path);
    result /= "AllMuteHotkey";
    return result;
}

std::filesystem::path config_file_path() {
    return config_directory() / "config.ini";
}

std::optional<Config> load_config() {
    Config config;
    std::ifstream file(config_file_path());
    if (!file) {
        return config;
    }

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }

        const auto sep = line.find('=');
        if (sep == std::string::npos) {
            continue;
        }

        const std::string key = trim(line.substr(0, sep));
        const std::string value = trim(line.substr(sep + 1));

        if (key == "hotkey_modifiers") {
            config.hotkey_modifiers = static_cast<std::uint32_t>(std::stoul(value, nullptr, 0));
        } else if (key == "hotkey_vk") {
            config.hotkey_vk = static_cast<std::uint32_t>(std::stoul(value, nullptr, 0));
        } else if (key == "autostart") {
            parse_bool(value, config.autostart);
        } else if (key == "notify_sound") {
            parse_bool(value, config.notify_sound);
        }
    }

    config.configured = config.hotkey_modifiers != 0 && config.hotkey_vk != 0;
    return config;
}

bool save_config(const Config& config) {
    const auto dir = config_directory();
    if (dir.empty()) {
        return false;
    }

    std::error_code ec;
    std::filesystem::create_directories(dir, ec);

    std::ofstream file(config_file_path(), std::ios::trunc);
    if (!file) {
        return false;
    }

    file << "# AllMuteHotkey configuration\n";
    file << "hotkey_modifiers=" << config.hotkey_modifiers << "\n";
    file << "hotkey_vk=" << config.hotkey_vk << "\n";
    file << "autostart=" << (config.autostart ? "1" : "0") << "\n";
    file << "notify_sound=" << (config.notify_sound ? "1" : "0") << "\n";
    return true;
}

}  // namespace amh

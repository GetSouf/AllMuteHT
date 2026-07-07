#pragma once

#include <filesystem>
#include <string>

namespace amh {

std::wstring executable_path();
bool is_autostart_enabled();
bool enable_autostart();
bool disable_autostart();

}  // namespace amh

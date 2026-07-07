#pragma once

#include "config.h"

#include <string>
#include <vector>

namespace amh {

void setup_console_utf8();
void print_help();
void print_first_run_hint();
void print_status(const Config& config);
void print_error(const std::wstring& message);
void print_success(const std::wstring& message);
void print_info(const std::wstring& message);

}  // namespace amh

#include "console_ui.h"

#include "audio.h"
#include "autostart.h"
#include "daemon.h"
#include "hotkey.h"

#include <iostream>
#include <windows.h>

namespace amh {

void setup_console_utf8() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
}

void print_help() {
    std::wcout << L"\nAllMuteHotkey — глобальный мут всех микрофонов по хоткею\n\n";
    std::wcout << L"Использование:\n";
    std::wcout << L"  AllMuteHotkey run                 Запустить фоновый режим (без окна)\n";
    std::wcout << L"  AllMuteHotkey status              Показать хоткей, автозагрузку и микрофоны\n";
    std::wcout << L"  AllMuteHotkey hotkey Ctrl+Shift+M Задать глобальный хоткей\n";
    std::wcout << L"  AllMuteHotkey toggle              Переключить мут всех микрофонов\n";
    std::wcout << L"  AllMuteHotkey mute                Выключить все микрофоны\n";
    std::wcout << L"  AllMuteHotkey unmute              Включить все микрофоны\n";
    std::wcout << L"  AllMuteHotkey install             Добавить в автозагрузку и запустить\n";
    std::wcout << L"  AllMuteHotkey uninstall           Убрать из автозагрузки и остановить\n";
    std::wcout << L"  AllMuteHotkey start               Запустить фоновый режим\n";
    std::wcout << L"  AllMuteHotkey stop                Остановить фоновый режим\n";
    std::wcout << L"  AllMuteHotkey notify on|off       Звук при переключении мутa\n";
    std::wcout << L"  AllMuteHotkey help                Показать эту справку\n\n";
    std::wcout << L"Примеры хоткеев: Ctrl+Shift+M, Ctrl+Alt+F8, Win+Shift+V\n";
    std::wcout << L"Нужен хотя бы один модификатор: Ctrl, Alt, Shift или Win.\n\n";
}

void print_first_run_hint() {
    std::wcout << L"Хоткей ещё не настроен.\n";
    std::wcout << L"1) AllMuteHotkey hotkey Ctrl+Shift+M\n";
    std::wcout << L"2) AllMuteHotkey install\n\n";
}

void print_status(const Config& config) {
    std::wcout << L"\n--- Статус AllMuteHotkey ---\n";
    std::wcout << L"Фоновый режим: " << (is_daemon_running() ? L"запущен" : L"остановлен") << L"\n";
    std::wcout << L"Автозагрузка: " << (is_autostart_enabled() ? L"включена" : L"выключена") << L"\n";
    std::wcout << L"Звук при переключении: " << (config.notify_sound ? L"вкл" : L"выкл") << L"\n";

    if (config.configured) {
        Hotkey hotkey{config.hotkey_modifiers, config.hotkey_vk};
        std::wcout << L"Хоткей: " << format_hotkey(hotkey) << L"\n";
    } else {
        std::wcout << L"Хоткей: не задан\n";
    }

    const auto mics = list_microphones();
    if (mics.empty()) {
        std::wcout << L"\nМикрофоны: активные устройства не найдены.\n";
        std::wcout << L"Подключите микрофон или проверьте настройки звука Windows.\n";
        return;
    }

    std::wcout << L"\nМикрофоны:\n";
    for (const auto& mic : mics) {
        std::wcout << L"  - " << mic.name << L": ";
        if (!mic.reachable) {
            std::wcout << L"недоступен\n";
        } else {
            std::wcout << (mic.muted ? L"выкл (mute)" : L"вкл") << L"\n";
        }
    }
    std::wcout << L"Общий статус: " << (are_all_microphones_muted() ? L"все выключены" : L"есть включённые")
               << L"\n";
}

void print_error(const std::wstring& message) {
    std::wcerr << L"Ошибка: " << message << L"\n";
}

void print_success(const std::wstring& message) {
    std::wcout << message << L"\n";
}

void print_info(const std::wstring& message) {
    std::wcout << message << L"\n";
}

}  // namespace amh

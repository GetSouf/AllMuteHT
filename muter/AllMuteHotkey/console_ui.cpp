#include "console_ui.h"

#include "audio.h"
#include "autostart.h"
#include "daemon.h"
#include "hotkey.h"

#include <iostream>
#include <limits>
#include <locale>
#include <windows.h>

namespace amh {

namespace {
UiLanguage g_ui_language = UiLanguage::Ru;

const wchar_t* tr(const wchar_t* ru, const wchar_t* en) {
    return g_ui_language == UiLanguage::En ? en : ru;
}
}  // namespace

void setup_console_utf8() {
    try {
        std::locale::global(std::locale(""));
        std::wcout.imbue(std::locale());
        std::wcin.imbue(std::locale());
        std::wcerr.imbue(std::locale());
    } catch (...) {
        // Fall back to default locale if system locale is unavailable.
    }
}

void set_ui_language(const std::string& code) {
    g_ui_language = (code == "en") ? UiLanguage::En : UiLanguage::Ru;
}

std::string get_ui_language_code() {
    return g_ui_language == UiLanguage::En ? "en" : "ru";
}

void print_help() {
    std::wcout << L"\n" << tr(L"AllMuteHotkey — глобальный мут всех микрофонов по хоткею",
                            L"AllMuteHotkey — global mute for all microphones")
               << L"\n\n";
    std::wcout << tr(L"Использование:\n", L"Usage:\n");
    std::wcout << tr(L"  AllMuteHotkey menu                Открыть меню с управлением по цифрам\n",
                     L"  AllMuteHotkey menu                Open numeric menu\n");
    std::wcout << tr(L"  AllMuteHotkey run                 Запустить фоновый режим (без окна)\n",
                     L"  AllMuteHotkey run                 Run daemon mode (hidden)\n");
    std::wcout << tr(L"  AllMuteHotkey status              Показать хоткей, автозагрузку и микрофоны\n",
                     L"  AllMuteHotkey status              Show hotkey, autostart and microphones\n");
    std::wcout << tr(L"  AllMuteHotkey hotkey Ctrl+Shift+M Задать глобальный хоткей\n",
                     L"  AllMuteHotkey hotkey Ctrl+Shift+M Set global hotkey\n");
    std::wcout << tr(L"  AllMuteHotkey lang ru|en          Переключить язык интерфейса\n",
                     L"  AllMuteHotkey lang ru|en          Switch interface language\n");
    std::wcout << tr(L"  AllMuteHotkey help                Показать эту справку\n\n",
                     L"  AllMuteHotkey help                Show this help\n\n");
    print_hotkey_examples();
    std::wcout << L"\n";
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

void print_menu(const Config& config) {
    std::wcout << L"\n=== AllMuteHotkey ===\n";
    std::wcout << tr(L"1. Показать статус\n", L"1. Show status\n");
    std::wcout << tr(L"2. Настроить хоткей\n", L"2. Configure hotkey\n");
    std::wcout << tr(L"3. Включить автозагрузку и запустить фон\n", L"3. Enable autostart and run daemon\n");
    std::wcout << tr(L"4. Выключить автозагрузку и остановить фон\n",
                     L"4. Disable autostart and stop daemon\n");
    std::wcout << tr(L"5. Запустить фоновый режим\n", L"5. Start daemon\n");
    std::wcout << tr(L"6. Остановить фоновый режим\n", L"6. Stop daemon\n");
    std::wcout << tr(L"7. Выключить все микрофоны\n", L"7. Mute all microphones\n");
    std::wcout << tr(L"8. Включить все микрофоны\n", L"8. Unmute all microphones\n");
    std::wcout << tr(L"9. Переключить mute/unmute\n", L"9. Toggle mute/unmute\n");
    std::wcout << tr(L"10. Переключить звук уведомления\n", L"10. Toggle notification sound\n");
    std::wcout << tr(L"11. Справка по командам\n", L"11. Help\n");
    std::wcout << tr(L"12. Переключить язык (RU/EN)\n", L"12. Switch language (RU/EN)\n");
    std::wcout << tr(L"0. Выход\n\n", L"0. Exit\n\n");

    if (config.configured) {
        Hotkey hotkey{config.hotkey_modifiers, config.hotkey_vk};
        std::wcout << L"Текущий хоткей: " << format_hotkey(hotkey) << L"\n";
    } else {
        std::wcout << L"Текущий хоткей: не задан\n";
    }
    std::wcout << L"Фон: " << (is_daemon_running() ? L"запущен" : L"остановлен") << L"\n";
    std::wcout << L"Автозагрузка: " << (is_autostart_enabled() ? L"включена" : L"выключена") << L"\n";
    std::wcout << L"Звук уведомления: " << (config.notify_sound ? L"вкл" : L"выкл") << L"\n\n";
}

void print_hotkey_examples() {
    std::wcout << L"Примеры хоткеев: Ctrl+Shift+M, Ctrl+Alt+F8, Win+Shift+V\n";
    std::wcout << L"Нужен хотя бы один модификатор: Ctrl, Alt, Shift или Win.\n";
}

std::wstring prompt_line(const std::wstring& label) {
    std::wcout << label;
    std::wstring value;
    std::getline(std::wcin, value);
    return value;
}

int prompt_menu_choice() {
    while (true) {
        std::wcout << tr(L"Введите номер пункта: ", L"Enter menu number: ");
        std::wstring line;
        if (!std::getline(std::wcin, line)) {
            return 0;
        }

        try {
            std::size_t pos = 0;
            const int value = std::stoi(line, &pos);
            if (pos == line.size()) {
                return value;
            }
        } catch (...) {
        }

        print_error(tr(L"Введите число из меню.", L"Enter a number from menu."));
    }
}

void print_error(const std::wstring& message) {
    std::wcerr << tr(L"Ошибка: ", L"Error: ") << message << L"\n";
}

void print_success(const std::wstring& message) {
    std::wcout << message << L"\n";
}

void print_info(const std::wstring& message) {
    std::wcout << message << L"\n";
}

}  // namespace amh

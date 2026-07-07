#include "hotkey.h"

#include <algorithm>
#include <cwctype>
#include <sstream>
#include <vector>

#include <windows.h>

namespace amh {

namespace {

std::wstring to_lower(std::wstring text) {
    std::transform(text.begin(), text.end(), text.begin(),
                   [](wchar_t ch) { return static_cast<wchar_t>(std::towlower(ch)); });
    return text;
}

std::vector<std::wstring> split(const std::wstring& text, wchar_t delimiter) {
    std::vector<std::wstring> parts;
    std::wstringstream stream(text);
    std::wstring part;
    while (std::getline(stream, part, delimiter)) {
        if (!part.empty()) {
            parts.push_back(part);
        }
    }
    return parts;
}

std::optional<std::uint32_t> parse_function_key(const std::wstring& token) {
    if (token.size() < 2 || (token[0] != L'f' && token[0] != L'F')) {
        return std::nullopt;
    }

    const int number = _wtoi(token.c_str() + 1);
    if (number < 1 || number > 24) {
        return std::nullopt;
    }
    return static_cast<std::uint32_t>(VK_F1 + (number - 1));
}

std::optional<std::uint32_t> parse_main_key(const std::wstring& token) {
    if (token.size() == 1) {
        const wchar_t ch = token[0];
        if ((ch >= L'a' && ch <= L'z') || (ch >= L'0' && ch <= L'9')) {
            const SHORT mapped = VkKeyScanW(ch);
            if (mapped != -1) {
                return static_cast<std::uint32_t>(LOBYTE(mapped));
            }
        }
        return std::nullopt;
    }

    if (token == L"space") {
        return VK_SPACE;
    }
    if (token == L"tab") {
        return VK_TAB;
    }
    if (token == L"enter" || token == L"return") {
        return VK_RETURN;
    }
    if (token == L"escape" || token == L"esc") {
        return VK_ESCAPE;
    }
    if (token == L"backspace") {
        return VK_BACK;
    }
    if (token == L"delete" || token == L"del") {
        return VK_DELETE;
    }
    if (token == L"insert" || token == L"ins") {
        return VK_INSERT;
    }
    if (token == L"home") {
        return VK_HOME;
    }
    if (token == L"end") {
        return VK_END;
    }
    if (token == L"pageup" || token == L"pgup") {
        return VK_PRIOR;
    }
    if (token == L"pagedown" || token == L"pgdn") {
        return VK_NEXT;
    }
    if (token == L"up") {
        return VK_UP;
    }
    if (token == L"down") {
        return VK_DOWN;
    }
    if (token == L"left") {
        return VK_LEFT;
    }
    if (token == L"right") {
        return VK_RIGHT;
    }

    return parse_function_key(token);
}

}  // namespace

std::optional<Hotkey> parse_hotkey(const std::wstring& text) {
    if (text.empty()) {
        return std::nullopt;
    }

    Hotkey hotkey;
    std::uint32_t main_key_count = 0;

    const auto parts = split(to_lower(text), L'+');
    for (const auto& raw_part : parts) {
        std::wstring part = raw_part;
        part.erase(std::remove(part.begin(), part.end(), L' '), part.end());
        if (part.empty()) {
            continue;
        }

        if (part == L"ctrl" || part == L"control") {
            hotkey.modifiers |= MOD_CONTROL;
        } else if (part == L"alt") {
            hotkey.modifiers |= MOD_ALT;
        } else if (part == L"shift") {
            hotkey.modifiers |= MOD_SHIFT;
        } else if (part == L"win" || part == L"super" || part == L"windows") {
            hotkey.modifiers |= MOD_WIN;
        } else {
            const auto vk = parse_main_key(part);
            if (!vk) {
                return std::nullopt;
            }
            hotkey.vk = *vk;
            ++main_key_count;
        }
    }

    if (main_key_count != 1) {
        return std::nullopt;
    }

    if (hotkey.modifiers == 0) {
        return std::nullopt;
    }

    return hotkey;
}

std::wstring format_hotkey(const Hotkey& hotkey) {
    std::wstring result;
    if (hotkey.modifiers & MOD_CONTROL) {
        result += L"Ctrl";
    }
    if (hotkey.modifiers & MOD_SHIFT) {
        if (!result.empty()) {
            result += L"+";
        }
        result += L"Shift";
    }
    if (hotkey.modifiers & MOD_ALT) {
        if (!result.empty()) {
            result += L"+";
        }
        result += L"Alt";
    }
    if (hotkey.modifiers & MOD_WIN) {
        if (!result.empty()) {
            result += L"+";
        }
        result += L"Win";
    }

    wchar_t key_name[64] = {};
    const UINT scan_code = MapVirtualKeyW(hotkey.vk, MAPVK_VK_TO_VSC);
  if (scan_code != 0 &&
        GetKeyNameTextW(static_cast<LONG>(scan_code << 16), key_name, 64) > 0) {
        if (!result.empty()) {
            result += L"+";
        }
        result += key_name;
        return result;
    }

    if (!result.empty()) {
        result += L"+";
    }
    result += L"VK" + std::to_wstring(hotkey.vk);
    return result;
}

std::wstring hotkey_error_message(DWORD error_code) {
    switch (error_code) {
        case ERROR_HOTKEY_ALREADY_REGISTERED:
            return L"Эта комбинация уже занята другим приложением. Выберите другую.";
        default:
            return L"Не удалось зарегистрировать хоткей (код ошибки " + std::to_wstring(error_code) +
                   L").";
    }
}

}  // namespace amh

#include "HotkeyManager.h"

#include "EditorState.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace AutoItPlus::Editor
{
    namespace
    {
        std::string ToLower(std::string value)
        {
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
                return static_cast<char>(std::tolower(ch));
            });
            return value;
        }

        std::string KeyLabel(ImGuiKey key)
        {
            if (key == ImGuiKey_Equal)
                return "+";
            if (key == ImGuiKey_Minus)
                return "-";
            if (key == ImGuiKey_KeypadAdd)
                return "Num+";
            if (key == ImGuiKey_KeypadSubtract)
                return "Num-";
            if (const char* name = ImGui::GetKeyName(key); name != nullptr && name[0] != '\0')
                return name;
            return "Unknown";
        }

        std::optional<ImGuiKey> KeyFromToken(const std::string& token)
        {
            const std::string value = ToLower(token);
            if (value == "+" || value == "plus")
                return ImGuiKey_Equal;
            if (value == "-" || value == "minus")
                return ImGuiKey_Minus;
            if (value == "numplus" || value == "keypadplus" || value == "kp+")
                return ImGuiKey_KeypadAdd;
            if (value == "numminus" || value == "keypadminus" || value == "kp-")
                return ImGuiKey_KeypadSubtract;
            if (value.size() >= 2 && value[0] == 'f')
            {
                bool digitsOnly = true;
                for (size_t index = 1; index < value.size(); ++index)
                {
                    if (!std::isdigit(static_cast<unsigned char>(value[index])))
                    {
                        digitsOnly = false;
                        break;
                    }
                }

                if (digitsOnly)
                {
                    const int functionIndex = std::stoi(value.substr(1));
                    if (functionIndex >= 1 && functionIndex <= 24)
                        return static_cast<ImGuiKey>(ImGuiKey_F1 + (functionIndex - 1));
                }
            }
            if (value == "equal") return ImGuiKey_Equal;
            if (value == "comma") return ImGuiKey_Comma;
            if (value == "period" || value == "dot") return ImGuiKey_Period;
            if (value == "slash") return ImGuiKey_Slash;
            if (value == "backslash") return ImGuiKey_Backslash;
            if (value == "semicolon") return ImGuiKey_Semicolon;
            if (value == "apostrophe" || value == "quote") return ImGuiKey_Apostrophe;
            if (value == "leftbracket" || value == "[") return ImGuiKey_LeftBracket;
            if (value == "rightbracket" || value == "]") return ImGuiKey_RightBracket;

            if (value.size() == 1)
            {
                const char ch = value[0];
                if (ch >= 'a' && ch <= 'z')
                    return static_cast<ImGuiKey>(ImGuiKey_A + (ch - 'a'));
                if (ch >= '0' && ch <= '9')
                    return static_cast<ImGuiKey>(ImGuiKey_0 + (ch - '0'));
            }

            return std::nullopt;
        }
    }

    void HotkeyManager::Clear()
    {
        mBindings.clear();
    }

    void HotkeyManager::Register(Binding binding)
    {
        for (auto& existing : mBindings)
        {
            if (existing.id == binding.id)
            {
                existing = std::move(binding);
                return;
            }
        }

        mBindings.push_back(std::move(binding));
    }

    const HotkeyManager::Binding* HotkeyManager::Find(const std::string& id) const
    {
        for (const auto& binding : mBindings)
        {
            if (binding.id == id)
                return &binding;
        }

        return nullptr;
    }

    std::optional<std::string> HotkeyManager::LabelFor(const std::string& id) const
    {
        if (const Binding* binding = Find(id))
            return binding->label;
        return std::nullopt;
    }

    bool HotkeyManager::Process(EditorState& state) const
    {
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantTextInput)
            return false;

        for (const auto& binding : mBindings)
        {
            if (binding.canExecute && !binding.canExecute(state))
                continue;

            if (!Matches(binding.chord, io))
                continue;

            binding.callback(state);
            return true;
        }

        return false;
    }

    std::map<std::string, std::string> HotkeyManager::ExportBindings() const
    {
        std::map<std::string, std::string> bindings;
        for (const auto& binding : mBindings)
            bindings.emplace(binding.id, FormatChord(binding.chord));
        return bindings;
    }

    std::string HotkeyManager::FormatChord(const HotkeyChord& chord)
    {
        std::ostringstream label;
        bool hasPrefix = false;
        const bool compactPlus = chord.key == ImGuiKey_Equal && chord.shift;
        auto append = [&](const char* part) {
            if (hasPrefix)
                label << "+";
            label << part;
            hasPrefix = true;
        };

        if (chord.ctrl)
            append("Ctrl");
        if (chord.shift && !compactPlus)
            append("Shift");
        if (chord.alt)
            append("Alt");
        if (chord.super)
            append("Super");

        append(KeyLabel(chord.key).c_str());
        return label.str();
    }

    std::optional<HotkeyChord> HotkeyManager::ParseChord(const std::string& text)
    {
        if (text.empty())
            return std::nullopt;

        std::vector<std::string> tokens;
        std::string current;
        for (size_t index = 0; index < text.size(); ++index)
        {
            const char ch = text[index];
            if (ch == '+')
            {
                if (!current.empty())
                {
                    tokens.push_back(current);
                    current.clear();
                }
                else
                {
                    tokens.emplace_back("+");
                }
                continue;
            }
            current.push_back(ch);
        }
        if (!current.empty())
            tokens.push_back(current);

        HotkeyChord chord;
        bool keySet = false;
        for (auto token : tokens)
        {
            token = ToLower(token);
            if (token == "ctrl" || token == "control")
            {
                chord.ctrl = true;
                continue;
            }
            if (token == "shift")
            {
                chord.shift = true;
                continue;
            }
            if (token == "alt")
            {
                chord.alt = true;
                continue;
            }
            if (token == "super" || token == "cmd" || token == "win")
            {
                chord.super = true;
                continue;
            }

            const auto key = KeyFromToken(token);
            if (!key.has_value())
                return std::nullopt;

            chord.key = *key;
            keySet = true;

            if (token == "+" || token == "plus")
                chord.shift = true;
        }

        if (!keySet)
            return std::nullopt;

        return chord;
    }

    bool HotkeyManager::Matches(const HotkeyChord& chord, const ImGuiIO& io)
    {
        if (chord.key == ImGuiKey_None)
            return false;

        if (io.KeyCtrl != chord.ctrl)
            return false;
        if (io.KeyShift != chord.shift)
            return false;
        if (io.KeyAlt != chord.alt)
            return false;
        if (io.KeySuper != chord.super)
            return false;

        return ImGui::IsKeyPressed(chord.key, false);
    }
}

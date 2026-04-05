#pragma once

#include "imgui.h"

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace AutoItPlus::Editor
{
    struct EditorState;

    struct HotkeyChord
    {
        ImGuiKey key = ImGuiKey_None;
        bool ctrl = false;
        bool shift = false;
        bool alt = false;
        bool super = false;
    };

    class HotkeyManager
    {
    public:
        using Callback = std::function<void(EditorState&)>;
        using Predicate = std::function<bool(const EditorState&)>;

        struct Binding
        {
            std::string id;
            HotkeyChord chord;
            std::string label;
            Callback callback;
            Predicate canExecute;
        };

        void Clear();
        void Register(Binding binding);
        const Binding* Find(const std::string& id) const;
        std::optional<std::string> LabelFor(const std::string& id) const;
        bool Process(EditorState& state) const;
        std::map<std::string, std::string> ExportBindings() const;

        static std::string FormatChord(const HotkeyChord& chord);
        static std::optional<HotkeyChord> ParseChord(const std::string& text);

    private:
        static bool Matches(const HotkeyChord& chord, const ImGuiIO& io);

        std::vector<Binding> mBindings;
    };
}

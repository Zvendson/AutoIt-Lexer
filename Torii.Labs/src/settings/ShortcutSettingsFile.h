#pragma once

#include "settings/SettingFile.h"

#include <unordered_map>

namespace AutoItPlus::Editor::Settings
{
    class ShortcutSettingsFile final : public SettingFile
    {
    public:
        explicit ShortcutSettingsFile(std::unordered_map<std::string, std::string>& shortcuts);

        static std::filesystem::path DefaultPath();

    protected:
        Json Serialize() const override;
        void DeserializeJson(const Json& json) override;

    private:
        std::unordered_map<std::string, std::string>& mShortcuts;
    };
}

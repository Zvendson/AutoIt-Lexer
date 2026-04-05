#include "settings/ShortcutSettingsFile.h"

#include <cstdlib>

namespace AutoItPlus::Editor::Settings
{
    ShortcutSettingsFile::ShortcutSettingsFile(std::unordered_map<std::string, std::string>& shortcuts)
        : SettingFile(DefaultPath())
        , mShortcuts(shortcuts)
    {
    }

    std::filesystem::path ShortcutSettingsFile::DefaultPath()
    {
#if defined(_WIN32)
        char* appData = nullptr;
        std::size_t appDataLength = 0;
        if (_dupenv_s(&appData, &appDataLength, "APPDATA") == 0 && appData != nullptr)
        {
            const std::filesystem::path root(appData);
            std::free(appData);
            const auto toriiPath = root / "Torii" / "shortcuts.json";
            const auto legacyPath = root / "AutoItPlus" / "shortcuts.json";
            return std::filesystem::exists(toriiPath) ? toriiPath : (std::filesystem::exists(legacyPath) ? legacyPath : toriiPath);
        }
#else
        if (const char* home = std::getenv("HOME"))
        {
            const std::filesystem::path root = std::filesystem::path(home) / ".config";
            const auto toriiPath = root / "Torii" / "shortcuts.json";
            const auto legacyPath = root / "AutoItPlus" / "shortcuts.json";
            return std::filesystem::exists(toriiPath) ? toriiPath : (std::filesystem::exists(legacyPath) ? legacyPath : toriiPath);
        }
#endif
        return std::filesystem::current_path() / "shortcuts.json";
    }

    ShortcutSettingsFile::Json ShortcutSettingsFile::Serialize() const
    {
        Json json = Json::object();
        for (const auto& [id, chord] : mShortcuts)
            json[id] = chord;
        return json;
    }

    void ShortcutSettingsFile::DeserializeJson(const Json& json)
    {
        if (!json.is_object())
            return;

        mShortcuts.clear();
        for (auto it = json.begin(); it != json.end(); ++it)
        {
            if (it.value().is_string())
                mShortcuts[it.key()] = it.value().get<std::string>();
        }
    }
}

#pragma once

#include "EditorState.h"
#include "settings/SettingFile.h"

namespace AutoItPlus::Editor::Settings
{
    class UserSettingsFile final : public SettingFile
    {
    public:
        explicit UserSettingsFile(EditorPreferences& preferences);

        static std::filesystem::path DefaultPath();

    protected:
        Json Serialize() const override;
        void DeserializeJson(const Json& json) override;

    private:
        EditorPreferences& mPreferences;
    };
}

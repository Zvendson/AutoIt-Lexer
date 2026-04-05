#pragma once

#include "EditorState.h"
#include "settings/SettingFile.h"

namespace AutoItPlus::Editor::Settings
{
    class ProjectSettingsFile final : public SettingFile
    {
    public:
        ProjectSettingsFile(ProjectState& project, const std::filesystem::path& projectFilePath);

    protected:
        Json Serialize() const override;
        void DeserializeJson(const Json& json) override;
        bool SupportsLegacyText() const override;
        void DeserializeLegacyText(const std::string& text) override;

    private:
        void ApplyStrictLayout();

        ProjectState& mProject;
    };
}

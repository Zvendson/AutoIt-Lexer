#pragma once

#include "EditorState.h"
#include "settings/SettingFile.h"

namespace AutoItPlus::Editor::Settings
{
    struct WorkspaceSessionData
    {
        std::vector<std::filesystem::path> openFiles;
        std::vector<std::filesystem::path> expandedDirectories;
        std::size_t currentIndex = 0;
        BuildConfiguration buildConfiguration = BuildConfiguration::Debug;
    };

    class WorkspaceSettingsFile final : public SettingFile
    {
    public:
        WorkspaceSettingsFile(const ProjectState& project, WorkspaceSessionData& data, const std::filesystem::path& path);

    protected:
        Json Serialize() const override;
        void DeserializeJson(const Json& json) override;
        bool SupportsLegacyText() const override;
        void DeserializeLegacyText(const std::string& text) override;

    private:
        std::filesystem::path ToAbsoluteProjectPath(const std::filesystem::path& path) const;
        std::filesystem::path ToRelativeProjectPath(const std::filesystem::path& path) const;

        const ProjectState& mProject;
        WorkspaceSessionData& mData;
    };
}

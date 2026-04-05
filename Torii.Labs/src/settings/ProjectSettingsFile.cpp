#include "settings/ProjectSettingsFile.h"

#include <sstream>

namespace AutoItPlus::Editor::Settings
{
    namespace
    {
        std::string Trim(const std::string& value)
        {
            const auto begin = value.find_first_not_of(" \t\r\n");
            if (begin == std::string::npos)
                return {};

            const auto end = value.find_last_not_of(" \t\r\n");
            return value.substr(begin, end - begin + 1U);
        }
    }

    ProjectSettingsFile::ProjectSettingsFile(ProjectState& project, const std::filesystem::path& projectFilePath)
        : SettingFile(std::filesystem::absolute(projectFilePath).lexically_normal())
        , mProject(project)
    {
        mProject.projectFilePath = GetPath();
        mProject.rootDirectory = mProject.projectFilePath.parent_path();
        if (mProject.name.empty())
            mProject.name = mProject.projectFilePath.stem().string();
    }

    ProjectSettingsFile::Json ProjectSettingsFile::Serialize() const
    {
        auto mainFile = mProject.mainFilePath.lexically_relative(mProject.rootDirectory);
        if (mainFile.empty())
            mainFile = std::filesystem::path("code") / "Main.aup";

        return Json{
            {"name", mProject.name},
            {"mainFile", mainFile.generic_string()},
            {"includeDirectories", mProject.includeDirectories},
            {"customRuleFiles", mProject.customRuleFiles}
        };
    }

    void ProjectSettingsFile::DeserializeJson(const Json& json)
    {
        mProject.name = json.value("name", mProject.name);
        if (const auto mainFile = json.find("mainFile"); mainFile != json.end() && mainFile->is_string())
            mProject.mainFilePath = mProject.rootDirectory / std::filesystem::path(mainFile->get<std::string>());
        mProject.includeDirectories = json.value("includeDirectories", mProject.includeDirectories);
        mProject.customRuleFiles = json.value("customRuleFiles", mProject.customRuleFiles);
        ApplyStrictLayout();
    }

    bool ProjectSettingsFile::SupportsLegacyText() const
    {
        return true;
    }

    void ProjectSettingsFile::DeserializeLegacyText(const std::string& text)
    {
        std::stringstream stream(text);
        std::string line;
        while (std::getline(stream, line))
        {
            const auto separator = line.find('=');
            if (separator == std::string::npos)
                continue;

            const auto key = Trim(line.substr(0, separator));
            const auto value = Trim(line.substr(separator + 1));

            if (key == "name")
                mProject.name = value;
            else if (key == "main" || key == "main_file")
                mProject.mainFilePath = mProject.rootDirectory / std::filesystem::path(value);
            else if (key == "include_dirs")
                mProject.includeDirectories = value;
            else if (key == "custom_rules")
                mProject.customRuleFiles = value;
        }

        ApplyStrictLayout();
    }

    void ProjectSettingsFile::ApplyStrictLayout()
    {
        if (mProject.mainFilePath.empty())
            mProject.mainFilePath = mProject.rootDirectory / "code" / "Main.aup";
        else if (!mProject.mainFilePath.is_absolute())
            mProject.mainFilePath = (mProject.rootDirectory / mProject.mainFilePath).lexically_normal();
        mProject.debugOutputFilePath = mProject.rootDirectory / "build" / "debug" / (mProject.name + ".au3");
        mProject.releaseOutputFilePath = mProject.rootDirectory / "build" / "release" / (mProject.name + ".au3");
    }
}

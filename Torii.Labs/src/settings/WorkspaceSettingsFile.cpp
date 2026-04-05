#include "settings/WorkspaceSettingsFile.h"

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

    WorkspaceSettingsFile::WorkspaceSettingsFile(const ProjectState& project, WorkspaceSessionData& data, const std::filesystem::path& path)
        : SettingFile(path)
        , mProject(project)
        , mData(data)
    {
    }

    WorkspaceSettingsFile::Json WorkspaceSettingsFile::Serialize() const
    {
        Json openFiles = Json::array();
        for (const auto& path : mData.openFiles)
            openFiles.push_back(ToRelativeProjectPath(path).generic_string());
        Json expandedDirectories = Json::array();
        for (const auto& path : mData.expandedDirectories)
            expandedDirectories.push_back(ToRelativeProjectPath(path).generic_string());

        return Json{
            {"buildConfiguration", mData.buildConfiguration == BuildConfiguration::Release ? "Release" : "Debug"},
            {"current", mData.currentIndex},
            {"openFiles", openFiles},
            {"expandedDirectories", expandedDirectories}
        };
    }

    void WorkspaceSettingsFile::DeserializeJson(const Json& json)
    {
        mData.currentIndex = json.value("current", mData.currentIndex);
        mData.buildConfiguration = json.value("buildConfiguration", std::string("Debug")) == "Release"
            ? BuildConfiguration::Release
            : BuildConfiguration::Debug;
        mData.openFiles.clear();
        mData.expandedDirectories.clear();
        for (const auto& entry : json.value("openFiles", Json::array()))
        {
            if (entry.is_string())
                mData.openFiles.push_back(ToAbsoluteProjectPath(entry.get<std::string>()));
        }
        for (const auto& entry : json.value("expandedDirectories", Json::array()))
        {
            if (entry.is_string())
                mData.expandedDirectories.push_back(ToAbsoluteProjectPath(entry.get<std::string>()));
        }
    }

    bool WorkspaceSettingsFile::SupportsLegacyText() const
    {
        return true;
    }

    void WorkspaceSettingsFile::DeserializeLegacyText(const std::string& text)
    {
        mData.openFiles.clear();
        mData.expandedDirectories.clear();
        std::stringstream stream(text);
        std::string line;
        while (std::getline(stream, line))
        {
            const auto separator = line.find('=');
            if (separator == std::string::npos)
                continue;

            const auto key = Trim(line.substr(0, separator));
            const auto value = Trim(line.substr(separator + 1));
            if (key == "open")
                mData.openFiles.push_back(ToAbsoluteProjectPath(value));
            else if (key == "current")
                mData.currentIndex = static_cast<std::size_t>(std::max(0, std::stoi(value)));
            else if (key == "build_config")
                mData.buildConfiguration = value == "Release" ? BuildConfiguration::Release : BuildConfiguration::Debug;
        }
    }

    std::filesystem::path WorkspaceSettingsFile::ToAbsoluteProjectPath(const std::filesystem::path& path) const
    {
        if (path.empty())
            return {};
        if (path.is_absolute())
            return path.lexically_normal();
        return (mProject.rootDirectory / path).lexically_normal();
    }

    std::filesystem::path WorkspaceSettingsFile::ToRelativeProjectPath(const std::filesystem::path& path) const
    {
        if (path.empty())
            return {};

        const auto absolute = std::filesystem::absolute(path).lexically_normal();
        const auto root = std::filesystem::absolute(mProject.rootDirectory).lexically_normal();
        const auto relative = absolute.lexically_relative(root);
        return relative.empty() ? absolute : relative;
    }
}

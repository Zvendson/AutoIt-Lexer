#include "settings/SettingFile.h"

#include <cctype>
#include <fstream>
#include <stdexcept>

namespace AutoItPlus::Editor::Settings
{
    SettingFile::SettingFile(std::filesystem::path path)
        : mPath(std::move(path))
    {
    }

    void SettingFile::Load()
    {
        if (!std::filesystem::exists(mPath))
            return;

        const std::string text = ReadTextFile(mPath);
        if (LooksLikeJson(text))
        {
            DeserializeJson(ParseJsonText(text));
            return;
        }

        if (!SupportsLegacyText())
            throw std::runtime_error("Unsupported non-JSON settings file: " + mPath.string());

        DeserializeLegacyText(text);
    }

    void SettingFile::Save() const
    {
        WriteTextFile(mPath, Serialize().dump(2));
    }

    bool SettingFile::Exists() const
    {
        return std::filesystem::exists(mPath);
    }

    const std::filesystem::path& SettingFile::GetPath() const
    {
        return mPath;
    }

    bool SettingFile::SupportsLegacyText() const
    {
        return false;
    }

    void SettingFile::DeserializeLegacyText(const std::string& text)
    {
        (void)text;
    }

    bool SettingFile::LooksLikeJson(const std::string& text)
    {
        for (const char ch : text)
        {
            if (ch == '{' || ch == '[')
                return true;
            if (!std::isspace(static_cast<unsigned char>(ch)))
                return false;
        }

        return false;
    }

    SettingFile::Json SettingFile::ParseJsonText(const std::string& text)
    {
        return Json::parse(text, nullptr, true, true);
    }

    std::string SettingFile::ReadTextFile(const std::filesystem::path& path)
    {
        std::ifstream input(path, std::ios::binary);
        if (!input.is_open())
            throw std::runtime_error("Could not open file: " + path.string());

        return std::string((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    }

    void SettingFile::WriteTextFile(const std::filesystem::path& path, const std::string& text)
    {
        if (path.has_parent_path())
            std::filesystem::create_directories(path.parent_path());

        std::ofstream output(path, std::ios::binary);
        if (!output.is_open())
            throw std::runtime_error("Could not write file: " + path.string());

        output << text;
    }
}

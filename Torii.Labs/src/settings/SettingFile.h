#pragma once

#include "json.hpp"

#include <filesystem>
#include <string>

namespace AutoItPlus::Editor::Settings
{
    class SettingFile
    {
    public:
        explicit SettingFile(std::filesystem::path path);
        virtual ~SettingFile() = default;

        void Load();
        void Save() const;

        bool Exists() const;
        const std::filesystem::path& GetPath() const;

    protected:
        using Json = nlohmann::json;

        virtual Json Serialize() const = 0;
        virtual void DeserializeJson(const Json& json) = 0;
        virtual bool SupportsLegacyText() const;
        virtual void DeserializeLegacyText(const std::string& text);

        static bool LooksLikeJson(const std::string& text);
        static Json ParseJsonText(const std::string& text);
        static std::string ReadTextFile(const std::filesystem::path& path);
        static void WriteTextFile(const std::filesystem::path& path, const std::string& text);

    private:
        std::filesystem::path mPath;
    };
}

#include "settings/UserSettingsFile.h"

#include <algorithm>
#include <cstdlib>

namespace AutoItPlus::Editor::Settings
{
    namespace
    {
        using Json = nlohmann::json;

        Json JsonFromColor(const ImVec4& color)
        {
            return Json::array({ color.x, color.y, color.z, color.w });
        }

        ImVec4 ColorFromJson(const Json& json, const ImVec4& fallback)
        {
            if (!json.is_array() || json.size() != 4)
                return fallback;

            return ImVec4(
                json[0].get<float>(),
                json[1].get<float>(),
                json[2].get<float>(),
                json[3].get<float>());
        }

        const char* ThemePresetName(ThemePreset preset)
        {
            switch (preset)
            {
                case ThemePreset::Custom: return "Custom";
                case ThemePreset::Torii: return "Torii";
                case ThemePreset::Midnight: return "Midnight";
                case ThemePreset::Forest: return "Forest";
                case ThemePreset::Daylight: return "Daylight";
            }

            return "Torii";
        }

        ThemePreset ThemePresetFromJson(const Json& json, ThemePreset fallback)
        {
            if (!json.is_string())
                return fallback;

            const std::string value = json.get<std::string>();
            if (value == "Custom") return ThemePreset::Custom;
            if (value == "Torii") return ThemePreset::Torii;
            if (value == "Midnight") return ThemePreset::Midnight;
            if (value == "Forest") return ThemePreset::Forest;
            if (value == "Daylight") return ThemePreset::Daylight;
            return fallback;
        }

        Json JsonFromSyntaxPalette(const SyntaxPaletteSettings& settings)
        {
            return Json{
                {"keyword", JsonFromColor(settings.keyword)},
                {"identifier", JsonFromColor(settings.identifier)},
                {"knownIdentifier", JsonFromColor(settings.knownIdentifier)},
                {"preprocessor", JsonFromColor(settings.preprocessor)},
                {"comment", JsonFromColor(settings.comment)},
                {"string", JsonFromColor(settings.string)},
                {"number", JsonFromColor(settings.number)},
                {"punctuation", JsonFromColor(settings.punctuation)},
                {"background", JsonFromColor(settings.background)},
                {"currentLine", JsonFromColor(settings.currentLine)},
                {"lineNumber", JsonFromColor(settings.lineNumber)},
                {"selection", JsonFromColor(settings.selection)},
                {"currentLineEdge", JsonFromColor(settings.currentLineEdge)}
            };
        }

        void ApplySyntaxPaletteJson(const Json& json, SyntaxPaletteSettings& settings)
        {
            if (!json.is_object())
                return;

            settings.keyword = ColorFromJson(json.value("keyword", Json{}), settings.keyword);
            settings.identifier = ColorFromJson(json.value("identifier", Json{}), settings.identifier);
            settings.knownIdentifier = ColorFromJson(json.value("knownIdentifier", Json{}), settings.knownIdentifier);
            settings.preprocessor = ColorFromJson(json.value("preprocessor", Json{}), settings.preprocessor);
            settings.comment = ColorFromJson(json.value("comment", Json{}), settings.comment);
            settings.string = ColorFromJson(json.value("string", Json{}), settings.string);
            settings.number = ColorFromJson(json.value("number", Json{}), settings.number);
            settings.punctuation = ColorFromJson(json.value("punctuation", Json{}), settings.punctuation);
            settings.background = ColorFromJson(json.value("background", Json{}), settings.background);
            settings.currentLine = ColorFromJson(json.value("currentLine", Json{}), settings.currentLine);
            settings.lineNumber = ColorFromJson(json.value("lineNumber", Json{}), settings.lineNumber);
            settings.selection = ColorFromJson(json.value("selection", Json{}), settings.selection);
            settings.currentLineEdge = ColorFromJson(json.value("currentLineEdge", Json{}), settings.currentLineEdge);
        }

        Json JsonFromUiTheme(const UiThemeSettings& settings)
        {
            return Json{
                {"accentColor", JsonFromColor(settings.accentColor)},
                {"accentSoftColor", JsonFromColor(settings.accentSoftColor)},
                {"panelColor", JsonFromColor(settings.panelColor)},
                {"panelAltColor", JsonFromColor(settings.panelAltColor)},
                {"windowBg", JsonFromColor(settings.windowBg)},
                {"menuBarBg", JsonFromColor(settings.menuBarBg)},
                {"borderColor", JsonFromColor(settings.borderColor)},
                {"headerHovered", JsonFromColor(settings.headerHovered)},
                {"buttonHovered", JsonFromColor(settings.buttonHovered)},
                {"buttonActive", JsonFromColor(settings.buttonActive)},
                {"frameBg", JsonFromColor(settings.frameBg)},
                {"frameBgHovered", JsonFromColor(settings.frameBgHovered)},
                {"tabColor", JsonFromColor(settings.tabColor)},
                {"tabHovered", JsonFromColor(settings.tabHovered)},
                {"titleBg", JsonFromColor(settings.titleBg)},
                {"titleBgActive", JsonFromColor(settings.titleBgActive)},
                {"separatorColor", JsonFromColor(settings.separatorColor)},
                {"resizeGrip", JsonFromColor(settings.resizeGrip)},
                {"sliderGrabActive", JsonFromColor(settings.sliderGrabActive)},
                {"textColor", JsonFromColor(settings.textColor)},
                {"textDisabledColor", JsonFromColor(settings.textDisabledColor)},
                {"popupBg", JsonFromColor(settings.popupBg)},
                {"iconPrimary", JsonFromColor(settings.iconPrimary)},
                {"iconSecondary", JsonFromColor(settings.iconSecondary)},
                {"iconNeutral", JsonFromColor(settings.iconNeutral)},
                {"iconSuccess", JsonFromColor(settings.iconSuccess)}
            };
        }

        void ApplyUiThemeJson(const Json& json, UiThemeSettings& settings)
        {
            if (!json.is_object())
                return;

            settings.accentColor = ColorFromJson(json.value("accentColor", Json{}), settings.accentColor);
            settings.accentSoftColor = ColorFromJson(json.value("accentSoftColor", Json{}), settings.accentSoftColor);
            settings.panelColor = ColorFromJson(json.value("panelColor", Json{}), settings.panelColor);
            settings.panelAltColor = ColorFromJson(json.value("panelAltColor", Json{}), settings.panelAltColor);
            settings.windowBg = ColorFromJson(json.value("windowBg", Json{}), settings.windowBg);
            settings.menuBarBg = ColorFromJson(json.value("menuBarBg", Json{}), settings.menuBarBg);
            settings.borderColor = ColorFromJson(json.value("borderColor", Json{}), settings.borderColor);
            settings.headerHovered = ColorFromJson(json.value("headerHovered", Json{}), settings.headerHovered);
            settings.buttonHovered = ColorFromJson(json.value("buttonHovered", Json{}), settings.buttonHovered);
            settings.buttonActive = ColorFromJson(json.value("buttonActive", Json{}), settings.buttonActive);
            settings.frameBg = ColorFromJson(json.value("frameBg", Json{}), settings.frameBg);
            settings.frameBgHovered = ColorFromJson(json.value("frameBgHovered", Json{}), settings.frameBgHovered);
            settings.tabColor = ColorFromJson(json.value("tabColor", Json{}), settings.tabColor);
            settings.tabHovered = ColorFromJson(json.value("tabHovered", Json{}), settings.tabHovered);
            settings.titleBg = ColorFromJson(json.value("titleBg", Json{}), settings.titleBg);
            settings.titleBgActive = ColorFromJson(json.value("titleBgActive", Json{}), settings.titleBgActive);
            settings.separatorColor = ColorFromJson(json.value("separatorColor", Json{}), settings.separatorColor);
            settings.resizeGrip = ColorFromJson(json.value("resizeGrip", Json{}), settings.resizeGrip);
            settings.sliderGrabActive = ColorFromJson(json.value("sliderGrabActive", Json{}), settings.sliderGrabActive);
            settings.textColor = ColorFromJson(json.value("textColor", Json{}), settings.textColor);
            settings.textDisabledColor = ColorFromJson(json.value("textDisabledColor", Json{}), settings.textDisabledColor);
            settings.popupBg = ColorFromJson(json.value("popupBg", Json{}), settings.popupBg);
            settings.iconPrimary = ColorFromJson(json.value("iconPrimary", Json{}), settings.iconPrimary);
            settings.iconSecondary = ColorFromJson(json.value("iconSecondary", Json{}), settings.iconSecondary);
            settings.iconNeutral = ColorFromJson(json.value("iconNeutral", Json{}), settings.iconNeutral);
            settings.iconSuccess = ColorFromJson(json.value("iconSuccess", Json{}), settings.iconSuccess);
        }
    }

    UserSettingsFile::UserSettingsFile(EditorPreferences& preferences)
        : SettingFile(DefaultPath())
        , mPreferences(preferences)
    {
    }

    std::filesystem::path UserSettingsFile::DefaultPath()
    {
#if defined(_WIN32)
        char* appData = nullptr;
        std::size_t appDataLength = 0;
        if (_dupenv_s(&appData, &appDataLength, "APPDATA") == 0 && appData != nullptr)
        {
            const std::filesystem::path root(appData);
            std::free(appData);
            const auto toriiPath = root / "Torii" / "editor-settings.json";
            const auto legacyPath = root / "AutoItPlus" / "editor-settings.json";
            return std::filesystem::exists(toriiPath) ? toriiPath : (std::filesystem::exists(legacyPath) ? legacyPath : toriiPath);
        }
#else
        if (const char* home = std::getenv("HOME"))
        {
            const std::filesystem::path root = std::filesystem::path(home) / ".config";
            const auto toriiPath = root / "Torii" / "editor-settings.json";
            const auto legacyPath = root / "AutoItPlus" / "editor-settings.json";
            return std::filesystem::exists(toriiPath) ? toriiPath : (std::filesystem::exists(legacyPath) ? legacyPath : toriiPath);
        }
#endif
        return std::filesystem::current_path() / "editor-settings.json";
    }

    UserSettingsFile::Json UserSettingsFile::Serialize() const
    {
        return Json{
            {"themePreset", ThemePresetName(mPreferences.themePreset)},
            {"autoIt", JsonFromSyntaxPalette(mPreferences.autoIt)},
            {"autoItPlus", JsonFromSyntaxPalette(mPreferences.autoItPlus)},
            {"uiTheme", JsonFromUiTheme(mPreferences.uiTheme)},
            {"previewMappingHighlight", JsonFromColor(mPreferences.previewMappingHighlight)},
            {"editorZoom", mPreferences.editorZoom},
            {"previewZoom", mPreferences.previewZoom},
            {"outputZoom", mPreferences.outputZoom},
            {"showWhitespace", mPreferences.showWhitespace},
            {"showLineNumbers", mPreferences.showLineNumbers}
        };
    }

    void UserSettingsFile::DeserializeJson(const Json& json)
    {
        if (!json.is_object())
            return;

        mPreferences.themePreset = ThemePresetFromJson(json.value("themePreset", Json{}), mPreferences.themePreset);
        ApplySyntaxPaletteJson(json.value("autoIt", Json{}), mPreferences.autoIt);
        ApplySyntaxPaletteJson(json.value("autoItPlus", Json{}), mPreferences.autoItPlus);
        ApplyUiThemeJson(json.value("uiTheme", Json{}), mPreferences.uiTheme);
        mPreferences.previewMappingHighlight = ColorFromJson(json.value("previewMappingHighlight", Json{}), mPreferences.previewMappingHighlight);
        mPreferences.editorZoom = std::clamp(json.value("editorZoom", mPreferences.editorZoom), 0.75f, 2.5f);
        mPreferences.previewZoom = std::clamp(json.value("previewZoom", mPreferences.previewZoom), 0.75f, 2.5f);
        mPreferences.outputZoom = std::clamp(json.value("outputZoom", mPreferences.outputZoom), 0.75f, 2.5f);
        mPreferences.showWhitespace = json.value("showWhitespace", mPreferences.showWhitespace);
        mPreferences.showLineNumbers = json.value("showLineNumbers", mPreferences.showLineNumbers);
    }
}

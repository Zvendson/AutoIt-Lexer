#include "EditorUi.h"

#include "AutoItSyntax.h"
#include "EditorServices.h"
#include "imgui_widgets/SocialLinkWidgets.h"

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <objbase.h>
#include <wincodec.h>
#endif

#include <GLFW/glfw3.h>

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4244 4456 4702)
#endif
#define NANOSVG_IMPLEMENTATION
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvg.h"
#include "nanosvgrast.h"
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "misc/cpp/imgui_stdlib.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
    using namespace AutoItPlus::Editor;

    void ApplyPreferences(EditorState& state);
    void SetupStyle(const EditorPreferences& preferences);
    std::string Trim(const std::string& value);

    struct ThemeDefinition
    {
        const char* label;
        SyntaxPaletteSettings autoIt;
        SyntaxPaletteSettings autoItPlus;
        ImVec4 previewMappingHighlight;
        UiThemeSettings uiTheme;
    };

    const char* ThemePresetLabel(ThemePreset preset)
    {
        switch (preset)
        {
            case ThemePreset::Custom: return "Custom";
            case ThemePreset::Torii: return "Torii";
            case ThemePreset::Umi: return "Umi";
            case ThemePreset::Midnight: return "Midnight";
            case ThemePreset::Forest: return "Forest";
            case ThemePreset::Daylight: return "Daylight";
        }

        return "Custom";
    }

    ThemeDefinition MakeThemeDefinition(ThemePreset preset)
    {
        switch (preset)
        {
            case ThemePreset::Torii:
                return ThemeDefinition{
                    .label = "Torii",
                    .autoIt = {
                        ImVec4(0.88f, 0.62f, 0.42f, 1.0f),
                        ImVec4(0.90f, 0.89f, 0.91f, 1.0f),
                        ImVec4(0.86f, 0.70f, 0.72f, 1.0f),
                        ImVec4(0.83f, 0.55f, 0.65f, 1.0f),
                        ImVec4(0.58f, 0.55f, 0.59f, 1.0f),
                        ImVec4(0.95f, 0.66f, 0.49f, 1.0f),
                        ImVec4(0.79f, 0.62f, 0.84f, 1.0f),
                        ImVec4(0.73f, 0.74f, 0.78f, 1.0f),
                        ImVec4(0.11f, 0.11f, 0.13f, 1.0f),
                        ImVec4(0.24f, 0.19f, 0.21f, 0.60f),
                        ImVec4(0.49f, 0.48f, 0.54f, 1.0f),
                        ImVec4(0.66f, 0.44f, 0.51f, 0.36f),
                        ImVec4(0.80f, 0.58f, 0.68f, 0.72f)
                    },
                    .autoItPlus = {
                        ImVec4(0.90f, 0.64f, 0.44f, 1.0f),
                        ImVec4(0.91f, 0.90f, 0.92f, 1.0f),
                        ImVec4(0.88f, 0.73f, 0.75f, 1.0f),
                        ImVec4(0.85f, 0.58f, 0.68f, 1.0f),
                        ImVec4(0.60f, 0.57f, 0.61f, 1.0f),
                        ImVec4(0.97f, 0.68f, 0.51f, 1.0f),
                        ImVec4(0.81f, 0.64f, 0.86f, 1.0f),
                        ImVec4(0.75f, 0.76f, 0.80f, 1.0f),
                        ImVec4(0.11f, 0.11f, 0.13f, 1.0f),
                        ImVec4(0.24f, 0.19f, 0.21f, 0.60f),
                        ImVec4(0.49f, 0.48f, 0.54f, 1.0f),
                        ImVec4(0.66f, 0.44f, 0.51f, 0.36f),
                        ImVec4(0.80f, 0.58f, 0.68f, 0.72f)
                    },
                    .previewMappingHighlight = ImVec4(0.80f, 0.52f, 0.60f, 0.20f),
                    .uiTheme = {
                        .accentColor = ImVec4(0.84f, 0.40f, 0.34f, 1.0f),
                        .accentSoftColor = ImVec4(0.32f, 0.20f, 0.24f, 1.0f),
                        .panelColor = ImVec4(0.22f, 0.16f, 0.18f, 1.0f),
                        .panelAltColor = ImVec4(0.26f, 0.19f, 0.21f, 1.0f),
                        .windowBg = ImVec4(0.16f, 0.12f, 0.14f, 1.0f),
                        .menuBarBg = ImVec4(0.20f, 0.14f, 0.16f, 1.0f),
                        .borderColor = ImVec4(0.40f, 0.30f, 0.33f, 1.0f),
                        .headerHovered = ImVec4(0.43f, 0.28f, 0.32f, 1.0f),
                        .buttonHovered = ImVec4(0.50f, 0.33f, 0.38f, 1.0f),
                        .buttonActive = ImVec4(0.58f, 0.38f, 0.42f, 1.0f),
                        .frameBg = ImVec4(0.27f, 0.19f, 0.21f, 1.0f),
                        .frameBgHovered = ImVec4(0.34f, 0.24f, 0.27f, 1.0f),
                        .tabColor = ImVec4(0.24f, 0.17f, 0.19f, 1.0f),
                        .tabHovered = ImVec4(0.37f, 0.26f, 0.29f, 1.0f),
                        .titleBg = ImVec4(0.22f, 0.16f, 0.18f, 1.0f),
                        .titleBgActive = ImVec4(0.30f, 0.21f, 0.24f, 1.0f),
                        .separatorColor = ImVec4(0.39f, 0.29f, 0.32f, 1.0f),
                        .resizeGrip = ImVec4(0.72f, 0.48f, 0.55f, 0.46f),
                        .sliderGrabActive = ImVec4(0.86f, 0.54f, 0.47f, 1.0f),
                        .textColor = ImVec4(0.92f, 0.88f, 0.89f, 1.0f),
                        .textDisabledColor = ImVec4(0.64f, 0.56f, 0.58f, 1.0f),
                        .popupBg = ImVec4(0.24f, 0.17f, 0.19f, 1.0f),
                        .iconPrimary = ImVec4(0.73f, 0.34f, 0.29f, 1.0f),
                        .iconSecondary = ImVec4(0.80f, 0.55f, 0.38f, 1.0f),
                        .iconNeutral = ImVec4(0.64f, 0.54f, 0.57f, 1.0f),
                        .iconSuccess = ImVec4(0.84f, 0.41f, 0.34f, 1.0f),
                        .directoryDropHighlight = ImVec4(0.78f, 0.42f, 0.46f, 0.16f),
                        .fileDropLine = ImVec4(0.88f, 0.58f, 0.60f, 0.88f)
                    }
                };
            case ThemePreset::Umi:
                return ThemeDefinition{
                    .label = "Umi",
                    .autoIt = {
                        ImVec4(0.76f, 0.80f, 0.98f, 1.0f),
                        ImVec4(0.88f, 0.91f, 0.95f, 1.0f),
                        ImVec4(0.66f, 0.82f, 0.88f, 1.0f),
                        ImVec4(0.50f, 0.71f, 0.90f, 1.0f),
                        ImVec4(0.54f, 0.58f, 0.64f, 1.0f),
                        ImVec4(0.58f, 0.76f, 0.94f, 1.0f),
                        ImVec4(0.70f, 0.74f, 0.93f, 1.0f),
                        ImVec4(0.74f, 0.78f, 0.84f, 1.0f),
                        ImVec4(0.10f, 0.11f, 0.15f, 1.0f),
                        ImVec4(0.18f, 0.22f, 0.29f, 0.60f),
                        ImVec4(0.49f, 0.57f, 0.68f, 1.0f),
                        ImVec4(0.38f, 0.52f, 0.68f, 0.34f),
                        ImVec4(0.58f, 0.72f, 0.90f, 0.72f)
                    },
                    .autoItPlus = {
                        ImVec4(0.80f, 0.84f, 0.99f, 1.0f),
                        ImVec4(0.90f, 0.92f, 0.96f, 1.0f),
                        ImVec4(0.70f, 0.85f, 0.92f, 1.0f),
                        ImVec4(0.54f, 0.75f, 0.94f, 1.0f),
                        ImVec4(0.57f, 0.61f, 0.67f, 1.0f),
                        ImVec4(0.62f, 0.80f, 0.97f, 1.0f),
                        ImVec4(0.73f, 0.77f, 0.95f, 1.0f),
                        ImVec4(0.76f, 0.80f, 0.86f, 1.0f),
                        ImVec4(0.10f, 0.11f, 0.15f, 1.0f),
                        ImVec4(0.18f, 0.22f, 0.29f, 0.60f),
                        ImVec4(0.52f, 0.60f, 0.71f, 1.0f),
                        ImVec4(0.40f, 0.54f, 0.71f, 0.34f),
                        ImVec4(0.61f, 0.75f, 0.93f, 0.72f)
                    },
                    .previewMappingHighlight = ImVec4(0.46f, 0.67f, 0.92f, 0.20f),
                    .uiTheme = {
                        .accentColor = ImVec4(0.30f, 0.56f, 0.88f, 1.0f),
                        .accentSoftColor = ImVec4(0.18f, 0.23f, 0.33f, 1.0f),
                        .panelColor = ImVec4(0.14f, 0.17f, 0.23f, 1.0f),
                        .panelAltColor = ImVec4(0.17f, 0.21f, 0.28f, 1.0f),
                        .windowBg = ImVec4(0.10f, 0.12f, 0.18f, 1.0f),
                        .menuBarBg = ImVec4(0.12f, 0.15f, 0.21f, 1.0f),
                        .borderColor = ImVec4(0.28f, 0.35f, 0.46f, 1.0f),
                        .headerHovered = ImVec4(0.25f, 0.36f, 0.52f, 1.0f),
                        .buttonHovered = ImVec4(0.30f, 0.43f, 0.60f, 1.0f),
                        .buttonActive = ImVec4(0.24f, 0.35f, 0.50f, 1.0f),
                        .frameBg = ImVec4(0.18f, 0.22f, 0.29f, 1.0f),
                        .frameBgHovered = ImVec4(0.22f, 0.27f, 0.36f, 1.0f),
                        .tabColor = ImVec4(0.16f, 0.20f, 0.27f, 1.0f),
                        .tabHovered = ImVec4(0.24f, 0.32f, 0.45f, 1.0f),
                        .titleBg = ImVec4(0.14f, 0.17f, 0.23f, 1.0f),
                        .titleBgActive = ImVec4(0.19f, 0.24f, 0.32f, 1.0f),
                        .separatorColor = ImVec4(0.27f, 0.35f, 0.45f, 1.0f),
                        .resizeGrip = ImVec4(0.44f, 0.60f, 0.82f, 0.42f),
                        .sliderGrabActive = ImVec4(0.40f, 0.68f, 0.98f, 1.0f),
                        .textColor = ImVec4(0.90f, 0.92f, 0.96f, 1.0f),
                        .textDisabledColor = ImVec4(0.61f, 0.67f, 0.77f, 1.0f),
                        .popupBg = ImVec4(0.15f, 0.18f, 0.25f, 1.0f),
                        .iconPrimary = ImVec4(0.50f, 0.72f, 0.98f, 1.0f),
                        .iconSecondary = ImVec4(0.64f, 0.82f, 0.96f, 1.0f),
                        .iconNeutral = ImVec4(0.70f, 0.76f, 0.84f, 1.0f),
                        .iconSuccess = ImVec4(0.38f, 0.74f, 0.94f, 1.0f),
                        .directoryDropHighlight = ImVec4(0.42f, 0.60f, 0.92f, 0.16f),
                        .fileDropLine = ImVec4(0.60f, 0.78f, 1.0f, 0.88f)
                    }
                };
            case ThemePreset::Forest:
                return ThemeDefinition{
                    .label = "Forest",
                    .autoIt = {
                        ImVec4(0.77f, 0.85f, 0.45f, 1.0f),
                        ImVec4(0.87f, 0.92f, 0.86f, 1.0f),
                        ImVec4(0.53f, 0.82f, 0.62f, 1.0f),
                        ImVec4(0.44f, 0.79f, 0.73f, 1.0f),
                        ImVec4(0.48f, 0.61f, 0.50f, 1.0f),
                        ImVec4(0.89f, 0.72f, 0.48f, 1.0f),
                        ImVec4(0.69f, 0.83f, 0.49f, 1.0f),
                        ImVec4(0.76f, 0.84f, 0.76f, 1.0f),
                        ImVec4(0.06f, 0.10f, 0.08f, 1.0f),
                        ImVec4(0.16f, 0.24f, 0.18f, 0.42f),
                        ImVec4(0.54f, 0.67f, 0.58f, 1.0f),
                        ImVec4(0.21f, 0.42f, 0.29f, 0.34f),
                        ImVec4(0.31f, 0.59f, 0.43f, 0.90f)
                    },
                    .autoItPlus = {
                        ImVec4(0.86f, 0.91f, 0.48f, 1.0f),
                        ImVec4(0.90f, 0.95f, 0.88f, 1.0f),
                        ImVec4(0.56f, 0.86f, 0.69f, 1.0f),
                        ImVec4(0.40f, 0.82f, 0.82f, 1.0f),
                        ImVec4(0.49f, 0.63f, 0.52f, 1.0f),
                        ImVec4(0.95f, 0.69f, 0.42f, 1.0f),
                        ImVec4(0.72f, 0.87f, 0.56f, 1.0f),
                        ImVec4(0.84f, 0.89f, 0.82f, 1.0f),
                        ImVec4(0.05f, 0.09f, 0.07f, 1.0f),
                        ImVec4(0.15f, 0.23f, 0.18f, 0.42f),
                        ImVec4(0.57f, 0.71f, 0.61f, 1.0f),
                        ImVec4(0.19f, 0.40f, 0.28f, 0.34f),
                        ImVec4(0.34f, 0.62f, 0.46f, 0.90f)
                    },
                    .previewMappingHighlight = ImVec4(0.44f, 0.79f, 0.58f, 0.28f),
                    .uiTheme = {
                        .accentColor = ImVec4(0.34f, 0.72f, 0.50f, 1.0f),
                        .accentSoftColor = ImVec4(0.16f, 0.30f, 0.22f, 1.0f),
                        .panelColor = ImVec4(0.08f, 0.12f, 0.10f, 1.0f),
                        .panelAltColor = ImVec4(0.10f, 0.15f, 0.12f, 1.0f),
                        .windowBg = ImVec4(0.05f, 0.08f, 0.06f, 1.0f),
                        .menuBarBg = ImVec4(0.07f, 0.11f, 0.08f, 1.0f),
                        .borderColor = ImVec4(0.18f, 0.27f, 0.21f, 1.0f),
                        .headerHovered = ImVec4(0.25f, 0.43f, 0.31f, 1.0f),
                        .buttonHovered = ImVec4(0.28f, 0.49f, 0.35f, 1.0f),
                        .buttonActive = ImVec4(0.20f, 0.38f, 0.27f, 1.0f),
                        .frameBg = ImVec4(0.10f, 0.14f, 0.12f, 1.0f),
                        .frameBgHovered = ImVec4(0.14f, 0.20f, 0.16f, 1.0f),
                        .tabColor = ImVec4(0.10f, 0.14f, 0.12f, 1.0f),
                        .tabHovered = ImVec4(0.18f, 0.29f, 0.22f, 1.0f),
                        .titleBg = ImVec4(0.08f, 0.12f, 0.10f, 1.0f),
                        .titleBgActive = ImVec4(0.10f, 0.15f, 0.12f, 1.0f),
                        .separatorColor = ImVec4(0.18f, 0.27f, 0.21f, 1.0f),
                        .resizeGrip = ImVec4(0.22f, 0.42f, 0.31f, 0.5f),
                        .sliderGrabActive = ImVec4(0.46f, 0.86f, 0.62f, 1.0f),
                        .textColor = ImVec4(0.88f, 0.92f, 0.90f, 1.0f),
                        .textDisabledColor = ImVec4(0.55f, 0.66f, 0.59f, 1.0f),
                        .popupBg = ImVec4(0.09f, 0.13f, 0.11f, 1.0f),
                        .iconPrimary = ImVec4(0.56f, 0.86f, 0.70f, 1.0f),
                        .iconSecondary = ImVec4(0.92f, 0.77f, 0.40f, 1.0f),
                        .iconNeutral = ImVec4(0.80f, 0.88f, 0.83f, 1.0f),
                        .iconSuccess = ImVec4(0.49f, 0.90f, 0.62f, 1.0f),
                        .directoryDropHighlight = ImVec4(0.36f, 0.66f, 0.50f, 0.16f),
                        .fileDropLine = ImVec4(0.63f, 0.84f, 0.58f, 0.88f)
                    }
                };
            case ThemePreset::Daylight:
                return ThemeDefinition{
                    .label = "Daylight",
                    .autoIt = {
                        ImVec4(0.65f, 0.45f, 0.12f, 1.0f),
                        ImVec4(0.14f, 0.20f, 0.28f, 1.0f),
                        ImVec4(0.12f, 0.46f, 0.70f, 1.0f),
                        ImVec4(0.12f, 0.54f, 0.78f, 1.0f),
                        ImVec4(0.42f, 0.48f, 0.56f, 1.0f),
                        ImVec4(0.76f, 0.36f, 0.22f, 1.0f),
                        ImVec4(0.42f, 0.38f, 0.74f, 1.0f),
                        ImVec4(0.17f, 0.23f, 0.31f, 1.0f),
                        ImVec4(0.98f, 0.99f, 1.00f, 1.0f),
                        ImVec4(0.78f, 0.87f, 0.98f, 0.82f),
                        ImVec4(0.48f, 0.56f, 0.66f, 1.0f),
                        ImVec4(0.60f, 0.79f, 0.96f, 0.28f),
                        ImVec4(0.34f, 0.55f, 0.83f, 0.86f)
                    },
                    .autoItPlus = {
                        ImVec4(0.73f, 0.47f, 0.14f, 1.0f),
                        ImVec4(0.12f, 0.19f, 0.27f, 1.0f),
                        ImVec4(0.11f, 0.50f, 0.74f, 1.0f),
                        ImVec4(0.14f, 0.58f, 0.82f, 1.0f),
                        ImVec4(0.39f, 0.46f, 0.55f, 1.0f),
                        ImVec4(0.79f, 0.38f, 0.24f, 1.0f),
                        ImVec4(0.47f, 0.39f, 0.78f, 1.0f),
                        ImVec4(0.16f, 0.22f, 0.30f, 1.0f),
                        ImVec4(0.99f, 0.99f, 1.00f, 1.0f),
                        ImVec4(0.76f, 0.86f, 0.98f, 0.84f),
                        ImVec4(0.46f, 0.54f, 0.64f, 1.0f),
                        ImVec4(0.56f, 0.78f, 0.95f, 0.30f),
                        ImVec4(0.32f, 0.53f, 0.80f, 0.84f)
                    },
                    .previewMappingHighlight = ImVec4(0.43f, 0.67f, 0.96f, 0.26f),
                    .uiTheme = {
                        .accentColor = ImVec4(0.28f, 0.56f, 0.88f, 1.0f),
                        .accentSoftColor = ImVec4(0.86f, 0.92f, 0.99f, 1.0f),
                        .panelColor = ImVec4(0.95f, 0.97f, 1.00f, 1.0f),
                        .panelAltColor = ImVec4(0.91f, 0.95f, 0.99f, 1.0f),
                        .windowBg = ImVec4(0.98f, 0.99f, 1.00f, 1.0f),
                        .menuBarBg = ImVec4(0.92f, 0.96f, 1.00f, 1.0f),
                        .borderColor = ImVec4(0.72f, 0.80f, 0.91f, 1.0f),
                        .headerHovered = ImVec4(0.78f, 0.88f, 0.99f, 1.0f),
                        .buttonHovered = ImVec4(0.72f, 0.85f, 0.98f, 1.0f),
                        .buttonActive = ImVec4(0.58f, 0.77f, 0.95f, 1.0f),
                        .frameBg = ImVec4(0.94f, 0.97f, 1.00f, 1.0f),
                        .frameBgHovered = ImVec4(0.88f, 0.93f, 0.99f, 1.0f),
                        .tabColor = ImVec4(0.92f, 0.95f, 0.99f, 1.0f),
                        .tabHovered = ImVec4(0.80f, 0.89f, 0.99f, 1.0f),
                        .titleBg = ImVec4(0.90f, 0.95f, 1.00f, 1.0f),
                        .titleBgActive = ImVec4(0.84f, 0.91f, 0.99f, 1.0f),
                        .separatorColor = ImVec4(0.75f, 0.82f, 0.92f, 1.0f),
                        .resizeGrip = ImVec4(0.33f, 0.57f, 0.86f, 0.45f),
                        .sliderGrabActive = ImVec4(0.32f, 0.61f, 0.94f, 1.0f),
                        .textColor = ImVec4(0.12f, 0.18f, 0.28f, 1.0f),
                        .textDisabledColor = ImVec4(0.45f, 0.53f, 0.63f, 1.0f),
                        .popupBg = ImVec4(0.96f, 0.98f, 1.00f, 1.0f),
                        .iconPrimary = ImVec4(0.20f, 0.49f, 0.84f, 1.0f),
                        .iconSecondary = ImVec4(0.74f, 0.56f, 0.20f, 1.0f),
                        .iconNeutral = ImVec4(0.36f, 0.47f, 0.60f, 1.0f),
                        .iconSuccess = ImVec4(0.19f, 0.63f, 0.47f, 1.0f),
                        .directoryDropHighlight = ImVec4(0.58f, 0.76f, 0.97f, 0.14f),
                        .fileDropLine = ImVec4(0.34f, 0.58f, 0.88f, 0.86f)
                    }
                };
            case ThemePreset::Midnight:
            case ThemePreset::Custom:
                return ThemeDefinition{
                    .label = "Midnight",
                    .autoIt = {},
                    .autoItPlus = {
                        ImVec4(0.92f, 0.72f, 0.24f, 1.0f),
                        ImVec4(0.89f, 0.90f, 0.94f, 1.0f),
                        ImVec4(0.51f, 0.84f, 0.70f, 1.0f),
                        ImVec4(0.35f, 0.78f, 0.98f, 1.0f),
                        ImVec4(0.48f, 0.53f, 0.60f, 1.0f),
                        ImVec4(0.95f, 0.58f, 0.37f, 1.0f),
                        ImVec4(0.76f, 0.64f, 0.95f, 1.0f),
                        ImVec4(0.80f, 0.84f, 0.90f, 1.0f),
                        ImVec4(0.05f, 0.07f, 0.10f, 1.0f),
                        ImVec4(0.15f, 0.20f, 0.26f, 0.42f)
                    },
                    .previewMappingHighlight = ImVec4(0.28f, 0.46f, 0.77f, 0.28f),
                    .uiTheme = {
                        .accentColor = ImVec4(0.20f, 0.55f, 0.93f, 1.0f),
                        .accentSoftColor = ImVec4(0.17f, 0.27f, 0.38f, 1.0f),
                        .panelColor = ImVec4(0.09f, 0.11f, 0.15f, 1.0f),
                        .panelAltColor = ImVec4(0.11f, 0.13f, 0.18f, 1.0f),
                        .windowBg = ImVec4(0.05f, 0.06f, 0.09f, 1.0f),
                        .menuBarBg = ImVec4(0.08f, 0.10f, 0.14f, 1.0f),
                        .borderColor = ImVec4(0.16f, 0.21f, 0.29f, 1.0f),
                        .headerHovered = ImVec4(0.24f, 0.39f, 0.58f, 1.0f),
                        .buttonHovered = ImVec4(0.25f, 0.43f, 0.63f, 1.0f),
                        .buttonActive = ImVec4(0.18f, 0.34f, 0.51f, 1.0f),
                        .frameBg = ImVec4(0.10f, 0.12f, 0.17f, 1.0f),
                        .frameBgHovered = ImVec4(0.14f, 0.18f, 0.25f, 1.0f),
                        .tabColor = ImVec4(0.11f, 0.14f, 0.19f, 1.0f),
                        .tabHovered = ImVec4(0.19f, 0.29f, 0.42f, 1.0f),
                        .titleBg = ImVec4(0.09f, 0.11f, 0.15f, 1.0f),
                        .titleBgActive = ImVec4(0.11f, 0.14f, 0.19f, 1.0f),
                        .separatorColor = ImVec4(0.16f, 0.21f, 0.29f, 1.0f),
                        .resizeGrip = ImVec4(0.18f, 0.31f, 0.47f, 0.5f),
                        .sliderGrabActive = ImVec4(0.31f, 0.65f, 1.0f, 1.0f),
                        .textColor = ImVec4(0.88f, 0.92f, 0.97f, 1.0f),
                        .textDisabledColor = ImVec4(0.56f, 0.63f, 0.73f, 1.0f),
                        .popupBg = ImVec4(0.08f, 0.10f, 0.14f, 1.0f),
                        .iconPrimary = ImVec4(0.59f, 0.82f, 1.0f, 1.0f),
                        .iconSecondary = ImVec4(0.96f, 0.76f, 0.36f, 1.0f),
                        .iconNeutral = ImVec4(0.82f, 0.86f, 0.93f, 1.0f),
                        .iconSuccess = ImVec4(0.40f, 0.86f, 0.64f, 1.0f),
                        .directoryDropHighlight = ImVec4(0.34f, 0.48f, 0.78f, 0.16f),
                        .fileDropLine = ImVec4(0.54f, 0.74f, 1.0f, 0.86f)
                    }
                };
        }

        return MakeThemeDefinition(ThemePreset::Torii);
    }

    void ApplyThemePreset(EditorPreferences& preferences, ThemePreset preset)
    {
        const auto definition = MakeThemeDefinition(preset);
        preferences.autoIt = definition.autoIt;
        preferences.autoItPlus = definition.autoItPlus;
        preferences.uiTheme = definition.uiTheme;
        preferences.previewMappingHighlight = definition.previewMappingHighlight;
        preferences.themePreset = preset;
    }

    const ThemeDefinition& ActiveTheme(const EditorPreferences& preferences)
    {
        static ThemeDefinition theme;
        theme = MakeThemeDefinition(preferences.themePreset == ThemePreset::Custom ? ThemePreset::Torii : preferences.themePreset);
        if (preferences.themePreset == ThemePreset::Custom)
        {
            theme.autoIt = preferences.autoIt;
            theme.autoItPlus = preferences.autoItPlus;
            theme.uiTheme = preferences.uiTheme;
            theme.previewMappingHighlight = preferences.previewMappingHighlight;
            theme.label = "Custom";
        }
        return theme;
    }

    struct EditorFonts
    {
        std::vector<float> sizes;
        std::vector<ImFont*> fonts;
    };

    EditorFonts g_uiFonts;
    EditorFonts g_editorFonts;

    struct IconTexture
    {
        GLuint handle = 0;
        int width = 0;
        int height = 0;
    };

    std::unordered_map<std::string, IconTexture> g_iconCache;

    std::filesystem::path FindEditorAssetRoot()
    {
        const auto sourceRoot = std::filesystem::path(__FILE__).parent_path().parent_path() / "assets";
        if (std::filesystem::exists(sourceRoot))
            return sourceRoot;

        const auto cwdRoot = std::filesystem::path("Torii.Labs") / "assets";
        if (std::filesystem::exists(cwdRoot))
            return cwdRoot;

        const auto legacyCwdRoot = std::filesystem::path("apps") / "AutoItPreprocessor.Editor" / "assets";
        if (std::filesystem::exists(legacyCwdRoot))
            return legacyCwdRoot;

        return {};
    }

    std::filesystem::path FindEditorFontPath(bool mono)
    {
        const auto assetRoot = FindEditorAssetRoot();
        if (assetRoot.empty())
            return {};
        const auto assetPath = assetRoot / "fonts" / (mono ? "NotoSansMono-VariableFont_wdth,wght.ttf" : "NotoSans-VariableFont_wdth,wght.ttf");
        if (std::filesystem::is_regular_file(assetPath))
            return assetPath;
        return {};
    }

    std::filesystem::path FindEditorIconPath(const std::string& iconName)
    {
        const auto assetRoot = FindEditorAssetRoot();
        if (assetRoot.empty())
            return {};

        // Reserved for future editor affordances: regex, quote, star, star-fill.
        static const std::unordered_map<std::string, std::string> kIconAliases = {
            {"build", "gear-fill"},
            {"file-symlink-file", "file-earmark-fill"},
            {"folder-opened", "folder2-open"},
            {"group-by-ref-type", "list-nested"},
            {"hashtag", "hash"},
            {"list-tree", "list-nested"},
            {"new-file", "file-earmark-plus-fill"},
            {"new-folder", "folder-plus"},
            {"output", "terminal-fill"},
            {"preview", "eye-fill"},
            {"project", "files"},
            {"root-folder-opened", "folder-fill"},
            {"run-all", "play-fill"},
            {"symbol-constant", "123"},
            {"symbol-keyword", "code-square"},
            {"symbol-method-arrow", "braces"},
            {"symbol-misc", "list"},
            {"symbol-namespace", "type"},
            {"symbol-parameter", "key-fill"},
            {"symbol-ruler", "rulers"},
            {"symbol-variable", "type"},
            {"wand", "magic"}
        };

        const auto tryIcon = [&](const std::string& candidateName) -> std::filesystem::path {
            const auto pngPath = assetRoot / "icons" / (candidateName + ".png");
            if (std::filesystem::is_regular_file(pngPath))
                return pngPath;
            const auto svgPath = assetRoot / "icons" / (candidateName + ".svg");
            if (std::filesystem::is_regular_file(svgPath))
                return svgPath;
            return {};
        };

        if (const auto direct = tryIcon(iconName); !direct.empty())
            return direct;

        if (const auto alias = kIconAliases.find(iconName); alias != kIconAliases.end())
        {
            if (const auto mapped = tryIcon(alias->second); !mapped.empty())
                return mapped;
        }

        return {};
    }

    std::string ToLower(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });
        return value;
    }

    const char* FindKnownFileTypeIcon(const std::filesystem::path& path)
    {
        if (!path.has_extension())
            return "file-earmark-fill";

        static const std::unordered_map<std::string, const char*> kExtensionIcons = {
            {".au3", "filetype-txt"},
            {".aup", "filetype-txt"},
            {".torii", "filetype-json"},
            {".bmp", "filetype-bmp"},
            {".cmd", "filetype-sh"},
            {".css", "filetype-css"},
            {".csv", "filetype-csv"},
            {".exe", "filetype-exe"},
            {".gif", "filetype-gif"},
            {".heic", "filetype-heic"},
            {".htm", "filetype-html"},
            {".html", "filetype-html"},
            {".ini", "filetype-txt"},
            {".java", "filetype-java"},
            {".jpeg", "filetype-jpg"},
            {".jpg", "filetype-jpg"},
            {".js", "filetype-js"},
            {".json", "filetype-json"},
            {".jsx", "filetype-jsx"},
            {".md", "filetype-md"},
            {".mdx", "filetype-mdx"},
            {".mp3", "filetype-mp3"},
            {".mp4", "filetype-mp4"},
            {".pdf", "filetype-pdf"},
            {".php", "filetype-php"},
            {".png", "filetype-png"},
            {".ps1", "filetype-sh"},
            {".psd", "filetype-psd"},
            {".py", "filetype-py"},
            {".rb", "filetype-rb"},
            {".sass", "filetype-sass"},
            {".scss", "filetype-scss"},
            {".sh", "filetype-sh"},
            {".sql", "filetype-sql"},
            {".svg", "filetype-svg"},
            {".tif", "filetype-tiff"},
            {".tiff", "filetype-tiff"},
            {".ts", "filetype-js"},
            {".tsx", "filetype-tsx"},
            {".ttf", "filetype-ttf"},
            {".txt", "filetype-txt"},
            {".wav", "filetype-wav"},
            {".woff", "filetype-woff"},
            {".xml", "filetype-xml"},
            {".xls", "filetype-xls"},
            {".xlsx", "filetype-xlsx"},
            {".yml", "filetype-yml"},
            {".yaml", "filetype-yml"}
        };

        const std::string extension = ToLower(path.extension().string());
        if (const auto it = kExtensionIcons.find(extension); it != kExtensionIcons.end())
            return it->second;
        return "file-earmark-fill";
    }

    void AdjustGlobalZoom(EditorState& state, float delta)
    {
        switch (state.activeZoomTarget)
        {
            case ZoomTarget::Source:
                state.preferences.editorZoom = std::clamp(state.preferences.editorZoom + delta, 0.75f, 2.5f);
                break;
            case ZoomTarget::Preview:
                state.preferences.previewZoom = std::clamp(state.preferences.previewZoom + delta, 0.75f, 2.5f);
                break;
            case ZoomTarget::Output:
                state.preferences.outputZoom = std::clamp(state.preferences.outputZoom + delta, 0.75f, 2.5f);
                break;
        }
        ApplyPreferences(state);
    }

    std::string ToSvgColor(const ImVec4& color)
    {
        char buffer[10] = {};
        std::snprintf(
            buffer,
            sizeof(buffer),
            "#%02X%02X%02X",
            std::clamp(static_cast<int>(std::lround(color.x * 255.0f)), 0, 255),
            std::clamp(static_cast<int>(std::lround(color.y * 255.0f)), 0, 255),
            std::clamp(static_cast<int>(std::lround(color.z * 255.0f)), 0, 255));
        return buffer;
    }

    void ReplaceAll(std::string& text, const std::string& from, const std::string& to)
    {
        if (from.empty())
            return;

        std::size_t start = 0;
        while ((start = text.find(from, start)) != std::string::npos)
        {
            text.replace(start, from.size(), to);
            start += to.size();
        }
    }

    IconTexture CreateTextureFromPixels(const unsigned char* pixels, int width, int height)
    {
        GLuint texture = 0;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        glBindTexture(GL_TEXTURE_2D, 0);
        return IconTexture{texture, width, height};
    }

#if defined(_WIN32)
    IconTexture LoadPngTexture(const std::filesystem::path& iconPath, const ImVec4& color)
    {
        HRESULT initResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        const bool didInitializeCom = SUCCEEDED(initResult);
        const bool canUseCom = didInitializeCom || initResult == RPC_E_CHANGED_MODE;
        if (!canUseCom)
            return {};

        IWICImagingFactory* factory = nullptr;
        IWICBitmapDecoder* decoder = nullptr;
        IWICBitmapFrameDecode* frame = nullptr;
        IWICFormatConverter* converter = nullptr;
        IconTexture iconTexture;
        UINT width = 0;
        UINT height = 0;
        std::vector<unsigned char> pixels;
        const unsigned char tintR = static_cast<unsigned char>(std::clamp(std::lround(color.x * 255.0f), 0L, 255L));
        const unsigned char tintG = static_cast<unsigned char>(std::clamp(std::lround(color.y * 255.0f), 0L, 255L));
        const unsigned char tintB = static_cast<unsigned char>(std::clamp(std::lround(color.z * 255.0f), 0L, 255L));
        const unsigned char tintA = static_cast<unsigned char>(std::clamp(std::lround(color.w * 255.0f), 0L, 255L));

        if (FAILED(CoCreateInstance(
                CLSID_WICImagingFactory,
                nullptr,
                CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(&factory))))
            goto cleanup;

        if (FAILED(factory->CreateDecoderFromFilename(
                iconPath.wstring().c_str(),
                nullptr,
                GENERIC_READ,
                WICDecodeMetadataCacheOnDemand,
                &decoder)))
            goto cleanup;

        if (FAILED(decoder->GetFrame(0, &frame)))
            goto cleanup;

        if (FAILED(factory->CreateFormatConverter(&converter)))
            goto cleanup;

        if (FAILED(converter->Initialize(
                frame,
                GUID_WICPixelFormat32bppRGBA,
                WICBitmapDitherTypeNone,
                nullptr,
                0.0,
                WICBitmapPaletteTypeCustom)))
            goto cleanup;

        if (FAILED(converter->GetSize(&width, &height)) || width == 0 || height == 0)
            goto cleanup;

        pixels.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U, 0);
        if (FAILED(converter->CopyPixels(nullptr, width * 4U, static_cast<UINT>(pixels.size()), pixels.data())))
            goto cleanup;

        for (std::size_t index = 0; index + 3U < pixels.size(); index += 4U)
        {
            const unsigned char sourceAlpha = pixels[index + 3U];
            const unsigned char finalAlpha = static_cast<unsigned char>((static_cast<unsigned int>(sourceAlpha) * static_cast<unsigned int>(tintA)) / 255U);
            pixels[index] = tintR;
            pixels[index + 1U] = tintG;
            pixels[index + 2U] = tintB;
            pixels[index + 3U] = finalAlpha;
        }

        iconTexture = CreateTextureFromPixels(pixels.data(), static_cast<int>(width), static_cast<int>(height));

    cleanup:
        if (converter != nullptr)
            converter->Release();
        if (frame != nullptr)
            frame->Release();
        if (decoder != nullptr)
            decoder->Release();
        if (factory != nullptr)
            factory->Release();
        if (didInitializeCom)
            CoUninitialize();
        return iconTexture;
    }
#endif

    const IconTexture* GetIconTexture(const std::string& iconName, int pixelSize, const ImVec4& color)
    {
        const auto iconPath = FindEditorIconPath(iconName);
        if (iconPath.empty())
            return nullptr;

        const bool isSvg = iconPath.extension() == ".svg";
        const auto cacheKey = isSvg
            ? iconName + "#" + std::to_string(pixelSize) + "#" + ToSvgColor(color)
            : iconName + "#png#" + ToSvgColor(color);
        if (const auto cached = g_iconCache.find(cacheKey); cached != g_iconCache.end())
            return cached->second.handle != 0 ? &cached->second : nullptr;

        if (!isSvg)
        {
#if defined(_WIN32)
            const auto [it, inserted] = g_iconCache.emplace(cacheKey, LoadPngTexture(iconPath, color));
            return inserted ? (it->second.handle != 0 ? &it->second : nullptr) : &g_iconCache.find(cacheKey)->second;
#else
            g_iconCache.emplace(cacheKey, IconTexture{});
            return nullptr;
#endif
        }

        auto svgText = ReadTextFile(iconPath);
        ReplaceAll(svgText, "currentColor", ToSvgColor(color));
        std::vector<char> svgBuffer(svgText.begin(), svgText.end());
        svgBuffer.push_back('\0');

        NSVGimage* image = nsvgParse(svgBuffer.data(), "px", 96.0f);
        if (image == nullptr)
        {
            g_iconCache.emplace(cacheKey, IconTexture{});
            return nullptr;
        }

        const float maxDimension = std::max(image->width, image->height);
        const float scale = maxDimension > 0.0f ? static_cast<float>(pixelSize) / maxDimension : 1.0f;
        const int width = std::max(1, static_cast<int>(std::ceil(image->width * scale)));
        const int height = std::max(1, static_cast<int>(std::ceil(image->height * scale)));
        std::vector<unsigned char> pixels(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U, 0);

        NSVGrasterizer* rasterizer = nsvgCreateRasterizer();
        nsvgRasterize(rasterizer, image, 0.0f, 0.0f, scale, pixels.data(), width, height, width * 4);

        nsvgDeleteRasterizer(rasterizer);
        nsvgDelete(image);

        const auto [it, inserted] = g_iconCache.emplace(cacheKey, CreateTextureFromPixels(pixels.data(), width, height));
        return inserted ? &it->second : &g_iconCache.find(cacheKey)->second;
    }

    void DestroyIconTextures()
    {
        for (auto& [key, icon] : g_iconCache)
        {
            if (icon.handle != 0)
                glDeleteTextures(1, &icon.handle);
        }
        g_iconCache.clear();
    }

    void LoadEditorFonts()
    {
        if (!g_editorFonts.fonts.empty() && !g_uiFonts.fonts.empty())
            return;

        ImGuiIO& io = ImGui::GetIO();
        const auto monoFontPath = FindEditorFontPath(true);
        const auto uiFontPath = FindEditorFontPath(false);
        if (monoFontPath.empty() || uiFontPath.empty())
            return;

        const std::array<float, 21> sizes = {
            12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f,
            23.0f, 24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 31.0f, 32.0f
        };
        static ImVector<ImWchar> extendedGlyphRanges;
        if (extendedGlyphRanges.empty())
        {
            ImFontGlyphRangesBuilder builder;
            builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
            static const ImWchar kExtraRanges[] = {
                0x00A0, 0x00FF, // Latin-1 supplement
                0x0100, 0x017F, // Latin extended-A
                0x0180, 0x024F, // Latin extended-B
                0x2000, 0x206F, // General punctuation
                0x2070, 0x209F, // Superscripts/Subscripts
                0x20A0, 0x20CF, // Currency symbols
                0x2190, 0x21FF, // Arrows
                0x2200, 0x22FF, // Mathematical operators
                0x2300, 0x23FF, // Misc technical
                0x2500, 0x257F, // Box drawing
                0x2580, 0x259F, // Block elements
                0x25A0, 0x25FF, // Geometric shapes
                0x2600, 0x26FF, // Misc symbols
                0x2700, 0x27BF, // Dingbats
                0,
            };
            builder.AddRanges(kExtraRanges);
            builder.BuildRanges(&extendedGlyphRanges);
        }

        ImFontConfig config;
        config.OversampleH = 4;
        config.OversampleV = 4;
        config.PixelSnapH = false;

        for (const float size : sizes)
        {
            if (ImFont* font = io.Fonts->AddFontFromFileTTF(monoFontPath.string().c_str(), size, &config, extendedGlyphRanges.Data))
            {
                g_editorFonts.sizes.push_back(size);
                g_editorFonts.fonts.push_back(font);
            }

            if (ImFont* font = io.Fonts->AddFontFromFileTTF(uiFontPath.string().c_str(), size, &config, extendedGlyphRanges.Data))
            {
                g_uiFonts.sizes.push_back(size);
                g_uiFonts.fonts.push_back(font);
            }
        }

        if (!g_uiFonts.fonts.empty())
            io.FontDefault = g_uiFonts.fonts[5];
    }

    ImFont* ChooseFont(const EditorFonts& fontSet, float zoom, float baseSize = 14.0f)
    {
        if (fontSet.fonts.empty())
            return nullptr;

        const float target = baseSize * zoom;
        std::size_t bestIndex = 0;
        float bestDistance = std::abs(fontSet.sizes[0] - target);
        for (std::size_t i = 1; i < fontSet.sizes.size(); ++i)
        {
            const float distance = std::abs(fontSet.sizes[i] - target);
            if (distance < bestDistance)
            {
                bestDistance = distance;
                bestIndex = i;
            }
        }

        return fontSet.fonts[bestIndex];
    }

    ImFont* ChooseUiFont(float zoom = 1.0f)
    {
        return ChooseFont(g_uiFonts, zoom, 18.0f);
    }

    ImFont* ChooseMonoFont(float zoom = 1.0f)
    {
        return ChooseFont(g_editorFonts, zoom, 15.0f);
    }

    void DrawIcon(const char* iconName, float size, const ImVec4& color)
    {
        if (const IconTexture* icon = GetIconTexture(iconName, static_cast<int>(std::ceil(size)), color))
        {
            ImGui::Image(
                (ImTextureID)(intptr_t)icon->handle,
                ImVec2(static_cast<float>(icon->width), static_cast<float>(icon->height)));
        }
    }

    void DrawInlineIcon(const char* iconName, float size, const ImVec4& color)
    {
        DrawIcon(iconName, size, color);
        ImGui::SameLine(0.0f, 8.0f);
    }

    struct FileTreeRowResult
    {
        bool hovered = false;
        bool clicked = false;
        bool doubleClicked = false;
        bool open = false;
        bool toggleOpen = false;
    };

    FileTreeRowResult DrawFileTreeRow(
        const std::string& id,
        const std::string& label,
        int depth,
        bool selected,
        bool isDirectory,
        bool open,
        const char* closedIconName,
        const char* openedIconName,
        const ImVec4& iconColor,
        const char* badgeText = nullptr,
        const ImVec4* badgeColor = nullptr)
    {
        constexpr float kRowHeight = 24.0f;
        constexpr float kIndentWidth = 18.0f;
        constexpr float kRowPaddingX = 12.0f;
        constexpr float kIconSize = 18.0f;
        constexpr float kIconTextGap = 8.0f;

        FileTreeRowResult result;

        ImGui::PushID(id.c_str());
        result.open = isDirectory ? open : false;

        const float rowWidth = std::max(1.0f, ImGui::GetContentRegionAvail().x);
        ImGui::InvisibleButton("##Row", ImVec2(rowWidth, kRowHeight));

        result.hovered = ImGui::IsItemHovered();
        result.clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
        result.doubleClicked = result.hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const ImVec2 itemMin = ImGui::GetItemRectMin();
        const ImVec2 itemMax = ImGui::GetItemRectMax();
        const ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
        const float indent = static_cast<float>(depth) * kIndentWidth;

        const ImVec2 iconMin(itemMin.x + kRowPaddingX + indent, itemMin.y + (kRowHeight - kIconSize) * 0.5f);
        const ImVec2 iconMax(iconMin.x + kIconSize, iconMin.y + kIconSize);
        const ImVec2 textMin(iconMax.x + kIconTextGap, itemMin.y + (kRowHeight - textSize.y) * 0.5f);

        if (selected || result.hovered)
        {
            const ImGuiCol colorIndex = selected
                ? (ImGui::IsMouseDown(ImGuiMouseButton_Left) && result.hovered ? ImGuiCol_HeaderActive : ImGuiCol_Header)
                : ImGuiCol_HeaderHovered;
            drawList->AddRectFilled(itemMin, itemMax, ImGui::GetColorU32(colorIndex), ImGui::GetStyle().FrameRounding);
        }

        const char* iconName = closedIconName;
        if (isDirectory && result.open)
            iconName = openedIconName;
        if (const IconTexture* icon = GetIconTexture(iconName, static_cast<int>(std::ceil(kIconSize)), iconColor))
        {
            const float iconY = itemMin.y + (kRowHeight - static_cast<float>(icon->height)) * 0.5f;
            const ImVec2 iconDrawMin(iconMin.x, iconY);
            const ImVec2 iconDrawMax(iconDrawMin.x + static_cast<float>(icon->width), iconDrawMin.y + static_cast<float>(icon->height));
            drawList->AddImage((ImTextureID)(intptr_t)icon->handle, iconDrawMin, iconDrawMax);
        }

        drawList->AddText(textMin, ImGui::GetColorU32(ImGuiCol_Text), label.c_str());

        if (badgeText != nullptr && badgeText[0] != '\0')
        {
            const ImVec2 badgeSize = ImGui::CalcTextSize(badgeText);
            const ImVec2 badgePos(itemMax.x - badgeSize.x - 10.0f, itemMin.y + (kRowHeight - badgeSize.y) * 0.5f);
            const ImU32 badgeTint = ImGui::GetColorU32(badgeColor != nullptr ? *badgeColor : ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
            drawList->AddText(badgePos, badgeTint, badgeText);
        }

        if (isDirectory && result.clicked)
        {
            const ImVec2 mousePos = ImGui::GetIO().MousePos;
            const bool iconClicked =
                mousePos.x >= iconMin.x && mousePos.x <= iconMax.x &&
                mousePos.y >= itemMin.y && mousePos.y <= itemMax.y;
            if (iconClicked || result.doubleClicked)
            {
                result.toggleOpen = true;
                result.open = !result.open;
            }
        }

        ImGui::PopID();
        return result;
    }

    bool DrawTreeSectionRow(const std::string& id, const std::string& label, int depth, const char* closedIconName, const char* openedIconName, const ImVec4& iconColor, bool defaultOpen = true)
    {
        return DrawFileTreeRow(id, label, depth, false, true, defaultOpen, closedIconName, openedIconName, iconColor).open;
    }

    void DrawSectionHeader(const char* iconName, const char* title, const std::string* subtitle = nullptr, ImVec4 color = kAccentColor)
    {
        DrawIcon(iconName, 16.0f, color);
        ImGui::SameLine(0.0f, 8.0f);
        ImGui::TextColored(color, "%s", title);
        if (subtitle != nullptr && !subtitle->empty())
        {
            ImGui::SameLine();
            ImGui::TextDisabled("%s", subtitle->c_str());
        }
    }

    bool DrawToolbarButton(
        const char* id,
        const char* iconName,
        const char* label,
        bool enabled = true,
        ImVec4 iconColor = ImVec4(0.87f, 0.90f, 0.96f, 1.0f),
        const ImVec4* buttonColor = nullptr,
        const ImVec4* buttonHoveredColor = nullptr,
        const ImVec4* buttonActiveColor = nullptr)
    {
        const ImVec2 textSize = ImGui::CalcTextSize(label);
        const bool iconOnly = label[0] == '\0';
        constexpr float kButtonHeight = 30.0f;
        constexpr float kHorizontalPadding = 12.0f;
        constexpr float kIconTextGap = 8.0f;
        const ImVec2 buttonSize = iconOnly ? ImVec2(kButtonHeight, kButtonHeight) : ImVec2(textSize.x + 14.0f + kIconTextGap + kHorizontalPadding * 2.0f, kButtonHeight);
        const std::string buttonId = std::string("##") + id;

        if (!enabled)
            ImGui::BeginDisabled();

        bool pushedButtonColors = false;
        if (buttonColor != nullptr && buttonHoveredColor != nullptr && buttonActiveColor != nullptr)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, *buttonColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, *buttonHoveredColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, *buttonActiveColor);
            pushedButtonColors = true;
        }

        const bool pressed = ImGui::Button(buttonId.c_str(), buttonSize);
        const ImVec2 rectMin = ImGui::GetItemRectMin();
        const ImVec2 rectMax = ImGui::GetItemRectMax();
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const ImU32 textColor = ImGui::GetColorU32(enabled ? ImGuiCol_Text : ImGuiCol_TextDisabled);

        const ImVec4 resolvedIconColor = enabled ? iconColor : ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
        if (const IconTexture* icon = GetIconTexture(iconName, 14, resolvedIconColor))
        {
            const float contentWidth = iconOnly
                ? static_cast<float>(icon->width)
                : static_cast<float>(icon->width) + kIconTextGap + textSize.x;
            const float contentStartX = rectMin.x + std::floor((buttonSize.x - contentWidth) * 0.5f);
            const float iconY = rectMin.y + std::floor((buttonSize.y - static_cast<float>(icon->height)) * 0.5f);
            const ImVec2 iconMin(contentStartX, iconY);
            const ImVec2 iconMax(iconMin.x + static_cast<float>(icon->width), iconMin.y + static_cast<float>(icon->height));
            drawList->AddImage((ImTextureID)(intptr_t)icon->handle, iconMin, iconMax);
            if (!iconOnly)
            {
                const float textX = iconMax.x + kIconTextGap;
                const float textY = rectMin.y + std::floor((buttonSize.y - textSize.y) * 0.5f);
                drawList->AddText(ImVec2(textX, textY), textColor, label);
            }
        }
        else if (!iconOnly)
        {
            const float textX = rectMin.x + std::floor((buttonSize.x - textSize.x) * 0.5f);
            const float textY = rectMin.y + std::floor((buttonSize.y - textSize.y) * 0.5f);
            drawList->AddText(ImVec2(textX, textY), textColor, label);
        }

        if (pushedButtonColors)
            ImGui::PopStyleColor(3);
        if (!enabled)
            ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && label[0] != '\0')
            ImGui::SetTooltip("%s", label);
        return enabled && pressed;
    }

    const char* FileActionPopupTitle(FileActionState::Kind kind)
    {
        switch (kind)
        {
            case FileActionState::Kind::NewFile:
                return "New File";
            case FileActionState::Kind::NewFolder:
                return "New Folder";
            case FileActionState::Kind::Rename:
                return "Rename";
            case FileActionState::Kind::Copy:
                return "Copy Item";
            case FileActionState::Kind::Duplicate:
                return "Duplicate Item";
            case FileActionState::Kind::Delete:
                return "Delete Item";
            case FileActionState::Kind::None:
                return nullptr;
        }

        return nullptr;
    }

    template <typename Callback>
    void ExecuteUiAction(EditorState& state, Callback&& callback)
    {
        try
        {
            callback();
            const auto message = HasOpenDocument(state) ? CurrentDocument(state).status : state.runStatus;
            const auto line = "[INFO] " + message + "\n";
            state.outputLog += line;
            if (state.outputEditor != nullptr)
                SetLoggerText(*state.outputEditor, state.outputLog);
        }
        catch (const std::exception& exception)
        {
            if (HasOpenDocument(state))
                CurrentDocument(state).status = exception.what();
            state.outputLog += "[ERROR] " + std::string(exception.what()) + "\n";
            if (state.outputEditor != nullptr)
                SetLoggerText(*state.outputEditor, state.outputLog);
        }
    }

    void JumpToLine(DocumentState& document, int line)
    {
        const int targetLine = std::max(0, line - 1);
        document.editor->SetCursorPosition(TextEditor::Coordinates(targetLine, 0));
        document.editor->RequestScrollToLineCentered(targetLine);

        if (document.previewEditor == nullptr)
            return;

        const std::size_t sourceLine = static_cast<std::size_t>(targetLine + 1);
        if (sourceLine >= document.previewLineMappings.size())
            return;

        const auto& mapping = document.previewLineMappings[sourceLine];
        if (mapping.generatedLineStart == 0 || mapping.generatedLineEnd == 0)
            return;

        const int previewLine = std::max(0, static_cast<int>(mapping.generatedLineStart - 1U));
        document.previewEditor->SetCursorPosition(TextEditor::Coordinates(previewLine, 0));
        document.previewEditor->RequestScrollToLineCentered(previewLine);
    }

    void SetUiStatus(EditorState& state, const std::string& message);
    void ConfigureDefaultHotkeys(EditorState& state);

    std::string ShortcutDisplayLabel(std::string id)
    {
        bool capitalize = true;
        for (char& ch : id)
        {
            if (ch == '_')
            {
                ch = ' ';
                capitalize = true;
                continue;
            }

            if (capitalize)
            {
                ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
                capitalize = false;
            }
        }

        return id;
    }

    void SyncShortcutEditBuffers(EditorState& state)
    {
        const auto bindings = state.hotkeys.ExportBindings();
        for (const auto& [id, chord] : bindings)
        {
            auto [it, inserted] = state.shortcutEditBuffers.try_emplace(id, chord);
            if (inserted || it->second.empty())
                it->second = chord;
        }
    }

    void RebuildHotkeys(EditorState& state)
    {
        state.hotkeys.Clear();
        ConfigureDefaultHotkeys(state);
        SyncShortcutEditBuffers(state);
        SaveShortcutOverrides(state.shortcutOverrides);
    }

    bool CanUndoCurrentDocument(const EditorState& state)
    {
        return HasOpenDocument(state) && CurrentDocument(state).editor != nullptr && CurrentDocument(state).editor->CanUndo();
    }

    bool CanRedoCurrentDocument(const EditorState& state)
    {
        return HasOpenDocument(state) && CurrentDocument(state).editor != nullptr && CurrentDocument(state).editor->CanRedo();
    }

    void UndoCurrentDocument(EditorState& state)
    {
        if (CanUndoCurrentDocument(state))
            CurrentDocument(state).editor->Undo();
    }

    void RedoCurrentDocument(EditorState& state)
    {
        if (CanRedoCurrentDocument(state))
            CurrentDocument(state).editor->Redo();
    }

    void FocusCurrentEditor(EditorState& state)
    {
        state.requestFocusCurrentEditor = true;
    }

    bool CloseDocumentWithPrompt(EditorState& state, std::size_t index)
    {
        if (index >= state.documents.size())
            return false;

        if (state.documents[index].dirty)
        {
            RequestAction(state, PendingAction::Kind::CloseDocument, index);
            return false;
        }

        if (state.outputEditor != nullptr)
            ApplySyntaxFlavor(*state.outputEditor, SyntaxFlavor::Logger, state.preferences);
        if (state.runEditor != nullptr)
            ApplySyntaxFlavor(*state.runEditor, SyntaxFlavor::Logger, state.preferences);

        CloseDocument(state, index);
        return true;
    }

    void CloseTabsToRight(EditorState& state, std::size_t index)
    {
        if (index + 1U >= state.documents.size())
            return;

        bool skippedDirty = false;
        for (std::size_t candidate = state.documents.size(); candidate-- > index + 1U;)
        {
            if (state.documents[candidate].dirty)
            {
                skippedDirty = true;
                continue;
            }

            CloseDocument(state, candidate);
        }

        if (skippedDirty)
            SetUiStatus(state, "Skipped dirty tabs while closing tabs to the right.");
    }

    void CloseOtherTabs(EditorState& state, std::size_t keepIndex)
    {
        bool skippedDirty = false;
        for (std::size_t candidate = state.documents.size(); candidate-- > 0U;)
        {
            if (candidate == keepIndex)
                continue;

            if (state.documents[candidate].dirty)
            {
                skippedDirty = true;
                continue;
            }

            CloseDocument(state, candidate);
            if (candidate < keepIndex)
                --keepIndex;
        }

        state.currentDocumentIndex = std::min(keepIndex, state.documents.empty() ? 0U : state.documents.size() - 1U);
        state.activateDocumentIndex = state.currentDocumentIndex;
        if (!state.documents.empty())
            FocusCurrentEditor(state);
        if (skippedDirty)
            SetUiStatus(state, "Skipped dirty tabs while closing other tabs.");
    }

    std::optional<std::filesystem::path> ResolveIncludePathForDocument(const EditorState& state, const DocumentState& document, const IncludeSymbol& include)
    {
        std::vector<std::filesystem::path> candidates;
        if (!include.isSystem)
            candidates.push_back((document.path.parent_path() / include.path).lexically_normal());

        if (state.project.has_value())
        {
            candidates.push_back((state.project->rootDirectory / include.path).lexically_normal());
            for (const auto& includeDir : SplitPaths(state.project->includeDirectories))
                candidates.push_back((MakeAbsoluteProjectPath(*state.project, includeDir) / include.path).lexically_normal());
        }

        for (const auto& candidate : candidates)
        {
            if (std::filesystem::exists(candidate))
                return std::filesystem::absolute(candidate).lexically_normal();
        }

        return std::nullopt;
    }

    void ApplyPreferences(EditorState& state)
    {
        SetupStyle(state.preferences);
        for (auto& document : state.documents)
        {
            if (document.editor != nullptr)
            {
                ApplySyntaxFlavor(*document.editor, document.sourceSyntax, state.preferences);
                document.editor->SetShowWhitespaces(state.preferences.showWhitespace);
                document.editor->SetShowLineNumbers(state.preferences.showLineNumbers);
            }
            if (document.previewEditor != nullptr)
            {
                ApplySyntaxFlavor(*document.previewEditor, SyntaxFlavor::AutoIt, state.preferences);
                document.previewEditor->SetShowWhitespaces(state.preferences.showWhitespace);
                document.previewEditor->SetShowLineNumbers(state.preferences.showLineNumbers);
            }
        }

        if (state.outputEditor != nullptr)
        {
            ApplySyntaxFlavor(*state.outputEditor, SyntaxFlavor::Logger, state.preferences);
            state.outputEditor->SetShowWhitespaces(false);
            state.outputEditor->SetShowLineNumbers(state.preferences.showLineNumbers);
        }

        if (state.runEditor != nullptr)
        {
            ApplySyntaxFlavor(*state.runEditor, SyntaxFlavor::Logger, state.preferences);
            state.runEditor->SetShowWhitespaces(false);
            state.runEditor->SetShowLineNumbers(state.preferences.showLineNumbers);
        }
        SaveEditorPreferences(state.preferences);
    }

    bool ShouldHideProjectPath(const ProjectState& project, const std::filesystem::path& path)
    {
        const auto relativePath = MakeProjectRelativePath(project, path);
        if (relativePath.empty())
            return false;

        const auto first = *relativePath.begin();
        return first == ".git" || first == "build" || first == ".torii" || first == ".autoit";
    }

    std::vector<ProjectTreeNode> BuildProjectTree(const ProjectState& project, const std::filesystem::path& path)
    {
        std::vector<ProjectTreeNode> nodes;
        std::vector<std::filesystem::directory_entry> entries;
        for (const auto& entry : std::filesystem::directory_iterator(path))
        {
            if (ShouldHideProjectPath(project, entry.path()))
                continue;
            entries.push_back(entry);
        }

        std::sort(entries.begin(), entries.end(), [](const auto& left, const auto& right) {
            if (left.is_directory() != right.is_directory())
                return left.is_directory() > right.is_directory();
            return left.path().filename().string() < right.path().filename().string();
        });

        nodes.reserve(entries.size());
        for (const auto& entry : entries)
        {
            ProjectTreeNode node;
            node.path = entry.path();
            node.name = entry.path().filename().empty() ? entry.path().string() : entry.path().filename().string();
            node.isDirectory = entry.is_directory();
            if (node.isDirectory)
                node.children = BuildProjectTree(project, entry.path());
            nodes.push_back(std::move(node));
        }

        return nodes;
    }

    void RequestProjectTreeRefresh(EditorState& state)
    {
        if (!state.project.has_value() || state.projectTreeLoading)
            return;

        const auto sourceRoot = std::filesystem::exists(state.project->rootDirectory / "code")
            ? state.project->rootDirectory / "code"
            : state.project->rootDirectory;
        state.projectTreeRoot = sourceRoot;
        state.projectTreeLoading = true;
        const auto project = *state.project;
        state.projectTreeTask = std::async(std::launch::async, [project, sourceRoot]() {
            return BuildProjectTree(project, sourceRoot);
        });
    }

    void PollProjectTreeRefresh(EditorState& state)
    {
        if (!state.projectTreeLoading || !state.projectTreeTask.valid())
            return;

        if (state.projectTreeTask.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
            return;

        state.projectTree = state.projectTreeTask.get();
        state.projectTreeLoading = false;
    }

    void DrawPreferences(EditorState& state)
    {
        if (!state.preferences.showPreferences)
            return;

        ImGui::SetNextWindowSize(ImVec2(560.0f, 520.0f), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Preferences", &state.preferences.showPreferences, ImGuiWindowFlags_NoCollapse))
            return;

        bool changed = false;
        if (ImGui::BeginTabBar("PreferenceTabs"))
        {
            auto drawPaletteTab = [&](const char* title, SyntaxPaletteSettings& settings) {
                if (!ImGui::BeginTabItem(title))
                    return;
                bool paletteChanged = false;
                paletteChanged |= ImGui::ColorEdit4("Keyword", &settings.keyword.x);
                paletteChanged |= ImGui::ColorEdit4("Identifier", &settings.identifier.x);
                paletteChanged |= ImGui::ColorEdit4("Known Id", &settings.knownIdentifier.x);
                paletteChanged |= ImGui::ColorEdit4("Preprocessor", &settings.preprocessor.x);
                paletteChanged |= ImGui::ColorEdit4("Comment", &settings.comment.x);
                paletteChanged |= ImGui::ColorEdit4("String", &settings.string.x);
                paletteChanged |= ImGui::ColorEdit4("Number", &settings.number.x);
                paletteChanged |= ImGui::ColorEdit4("Punctuation", &settings.punctuation.x);
                paletteChanged |= ImGui::ColorEdit4("Background", &settings.background.x);
                paletteChanged |= ImGui::ColorEdit4("Current Line", &settings.currentLine.x);
                paletteChanged |= ImGui::ColorEdit4("Line Number", &settings.lineNumber.x);
                paletteChanged |= ImGui::ColorEdit4("Selection", &settings.selection.x);
                paletteChanged |= ImGui::ColorEdit4("Current Line Edge", &settings.currentLineEdge.x);
                changed |= paletteChanged;
                if (paletteChanged)
                    state.preferences.themePreset = ThemePreset::Custom;
                ImGui::EndTabItem();
            };

            drawPaletteTab("AutoIt", state.preferences.autoIt);
            drawPaletteTab("AutoIt+", state.preferences.autoItPlus);

            if (ImGui::BeginTabItem("Editor"))
            {
                const ThemePreset themeOptions[] = {
                    ThemePreset::Torii,
                    ThemePreset::Umi,
                    ThemePreset::Midnight,
                    ThemePreset::Forest,
                    ThemePreset::Daylight
                };
                int selectedThemeIndex = 0;
                for (int index = 0; index < static_cast<int>(std::size(themeOptions)); ++index)
                {
                    if (state.preferences.themePreset == themeOptions[index])
                    {
                        selectedThemeIndex = index;
                        break;
                    }
                }

                const char* selectedThemeLabel =
                    state.preferences.themePreset == ThemePreset::Custom
                    ? ThemePresetLabel(ThemePreset::Custom)
                    : ThemePresetLabel(themeOptions[selectedThemeIndex]);
                if (ImGui::BeginCombo("Theme", selectedThemeLabel))
                {
                    for (int index = 0; index < static_cast<int>(std::size(themeOptions)); ++index)
                    {
                        const bool selected = state.preferences.themePreset == themeOptions[index];
                        if (ImGui::Selectable(ThemePresetLabel(themeOptions[index]), selected))
                        {
                            ApplyThemePreset(state.preferences, themeOptions[index]);
                            changed = true;
                        }
                        if (selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                changed |= ImGui::ColorEdit4("Preview Mapping", &state.preferences.previewMappingHighlight.x);
                if (ImGui::IsItemEdited())
                    state.preferences.themePreset = ThemePreset::Custom;
                changed |= ImGui::SliderFloat("Source Zoom", &state.preferences.editorZoom, 0.75f, 2.5f);
                changed |= ImGui::SliderFloat("Preview Zoom", &state.preferences.previewZoom, 0.75f, 2.5f);
                changed |= ImGui::SliderFloat("Output Zoom", &state.preferences.outputZoom, 0.75f, 2.5f);
                changed |= ImGui::Checkbox("Show Whitespace", &state.preferences.showWhitespace);
                changed |= ImGui::Checkbox("Show Line Numbers", &state.preferences.showLineNumbers);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Shell"))
            {
                bool shellChanged = false;
                shellChanged |= ImGui::ColorEdit4("Accent", &state.preferences.uiTheme.accentColor.x);
                shellChanged |= ImGui::ColorEdit4("Accent Soft", &state.preferences.uiTheme.accentSoftColor.x);
                shellChanged |= ImGui::ColorEdit4("Window", &state.preferences.uiTheme.windowBg.x);
                shellChanged |= ImGui::ColorEdit4("Panel", &state.preferences.uiTheme.panelColor.x);
                shellChanged |= ImGui::ColorEdit4("Panel Alt", &state.preferences.uiTheme.panelAltColor.x);
                shellChanged |= ImGui::ColorEdit4("Menu Bar", &state.preferences.uiTheme.menuBarBg.x);
                shellChanged |= ImGui::ColorEdit4("Border", &state.preferences.uiTheme.borderColor.x);
                shellChanged |= ImGui::ColorEdit4("Text", &state.preferences.uiTheme.textColor.x);
                shellChanged |= ImGui::ColorEdit4("Text Disabled", &state.preferences.uiTheme.textDisabledColor.x);
                shellChanged |= ImGui::ColorEdit4("Popup", &state.preferences.uiTheme.popupBg.x);
                shellChanged |= ImGui::ColorEdit4("Icon Primary", &state.preferences.uiTheme.iconPrimary.x);
                shellChanged |= ImGui::ColorEdit4("Icon Secondary", &state.preferences.uiTheme.iconSecondary.x);
                shellChanged |= ImGui::ColorEdit4("Icon Neutral", &state.preferences.uiTheme.iconNeutral.x);
                shellChanged |= ImGui::ColorEdit4("Icon Success", &state.preferences.uiTheme.iconSuccess.x);
                shellChanged |= ImGui::ColorEdit4("Drop Directory", &state.preferences.uiTheme.directoryDropHighlight.x);
                shellChanged |= ImGui::ColorEdit4("Drop File Line", &state.preferences.uiTheme.fileDropLine.x);
                shellChanged |= ImGui::ColorEdit4("Button Hover", &state.preferences.uiTheme.buttonHovered.x);
                shellChanged |= ImGui::ColorEdit4("Button Active", &state.preferences.uiTheme.buttonActive.x);
                shellChanged |= ImGui::ColorEdit4("Frame", &state.preferences.uiTheme.frameBg.x);
                shellChanged |= ImGui::ColorEdit4("Frame Hover", &state.preferences.uiTheme.frameBgHovered.x);
                shellChanged |= ImGui::ColorEdit4("Tab", &state.preferences.uiTheme.tabColor.x);
                shellChanged |= ImGui::ColorEdit4("Tab Hover", &state.preferences.uiTheme.tabHovered.x);
                shellChanged |= ImGui::ColorEdit4("Title", &state.preferences.uiTheme.titleBg.x);
                shellChanged |= ImGui::ColorEdit4("Title Active", &state.preferences.uiTheme.titleBgActive.x);
                shellChanged |= ImGui::ColorEdit4("Separator", &state.preferences.uiTheme.separatorColor.x);
                shellChanged |= ImGui::ColorEdit4("Resize Grip", &state.preferences.uiTheme.resizeGrip.x);
                shellChanged |= ImGui::ColorEdit4("Header Hover", &state.preferences.uiTheme.headerHovered.x);
                shellChanged |= ImGui::ColorEdit4("Slider Active", &state.preferences.uiTheme.sliderGrabActive.x);
                changed |= shellChanged;
                if (shellChanged)
                    state.preferences.themePreset = ThemePreset::Custom;
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Shortcuts"))
            {
                ConfigureDefaultHotkeys(state);
                SyncShortcutEditBuffers(state);

                ImGui::TextDisabled("Examples: ctrl+s, ctrl+shift+s, ctrl++, ctrl+minus, f7");
                if (ImGui::Button("Reset All Shortcuts"))
                {
                    state.shortcutOverrides.clear();
                    state.shortcutEditBuffers.clear();
                    RebuildHotkeys(state);
                    SetUiStatus(state, "Reset shortcuts to defaults.");
                }
                ImGui::Separator();

                if (ImGui::BeginTable("ShortcutBindings", 4, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
                {
                    ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch, 1.5f);
                    ImGui::TableSetupColumn("Shortcut", ImGuiTableColumnFlags_WidthStretch, 1.4f);
                    ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthStretch, 1.0f);
                    ImGui::TableSetupColumn("Apply", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                    ImGui::TableHeadersRow();

                    const auto bindings = state.hotkeys.ExportBindings();
                    for (const auto& [id, currentChord] : bindings)
                    {
                        std::string& editBuffer = state.shortcutEditBuffers[id];
                        if (editBuffer.empty())
                            editBuffer = currentChord;

                        const auto parsedChord = HotkeyManager::ParseChord(editBuffer);
                        const bool valid = parsedChord.has_value();
                        const bool changedBinding = editBuffer != currentChord;

                        ImGui::TableNextRow();

                        ImGui::TableSetColumnIndex(0);
                        ImGui::TextUnformatted(ShortcutDisplayLabel(id).c_str());

                        ImGui::TableSetColumnIndex(1);
                        ImGui::SetNextItemWidth(-1.0f);
                        ImGui::InputText(("##Shortcut_" + id).c_str(), &editBuffer);

                        ImGui::TableSetColumnIndex(2);
                        if (!valid)
                            ImGui::TextColored(ImVec4(0.92f, 0.38f, 0.38f, 1.0f), "Invalid");
                        else if (changedBinding)
                            ImGui::TextColored(ImVec4(0.88f, 0.72f, 0.32f, 1.0f), "Pending");
                        else
                            ImGui::TextDisabled("Active");

                        ImGui::TableSetColumnIndex(3);
                        const bool canApply = valid && changedBinding;
                        if (!canApply)
                            ImGui::BeginDisabled();
                        if (ImGui::Button(("Apply##" + id).c_str(), ImVec2(-1.0f, 0.0f)))
                        {
                            state.shortcutOverrides[id] = HotkeyManager::FormatChord(*parsedChord);
                            RebuildHotkeys(state);
                            state.shortcutEditBuffers[id] = state.shortcutOverrides[id];
                            SetUiStatus(state, "Updated shortcut for " + ShortcutDisplayLabel(id) + ".");
                        }
                        if (!canApply)
                            ImGui::EndDisabled();
                    }

                    ImGui::EndTable();
                }

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        if (changed)
            ApplyPreferences(state);

        if (ImGui::Button("Close", ImVec2(120.0f, 0.0f)))
            state.preferences.showPreferences = false;

        ImGui::End();
    }

    void OpenProjectSettings(EditorState& state)
    {
        if (!state.project.has_value())
            return;

        state.projectSettingsDialog.show = true;
        state.projectSettingsDialog.mainFilePath = MakeProjectRelativePath(*state.project, state.project->mainFilePath).generic_string();
        if (state.projectSettingsDialog.mainFilePath.empty())
            state.projectSettingsDialog.mainFilePath = (std::filesystem::path("code") / "Main.aup").generic_string();
    }

    void DrawProjectSettings(EditorState& state)
    {
        if (!state.project.has_value() || !state.projectSettingsDialog.show)
            return;

        auto& project = *state.project;
        auto& dialog = state.projectSettingsDialog;

        ImGui::SetNextWindowSize(ImVec2(520.0f, 220.0f), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Project Settings", &dialog.show, ImGuiWindowFlags_NoCollapse))
            return;

        ImGui::TextDisabled("%s", project.projectFilePath.string().c_str());
        ImGui::Separator();
        ImGui::TextWrapped("Set the main source file used for build and run. The path must stay inside the project `code/` directory.");
        ImGui::Spacing();

        ImGui::TextUnformatted("Main File");
        ImGui::InputText("##ProjectMainFile", &dialog.mainFilePath);

        const auto trimmedInput = Trim(dialog.mainFilePath);
        const auto codeDirectory = project.rootDirectory / "code";
        const auto candidateRelative = std::filesystem::path(trimmedInput);
        const auto candidateAbsolute = candidateRelative.is_absolute()
            ? candidateRelative.lexically_normal()
            : (project.rootDirectory / candidateRelative).lexically_normal();
        const auto candidateWithinCode = candidateAbsolute.lexically_relative(codeDirectory.lexically_normal());
        const bool inputEmpty = trimmedInput.empty();
        const bool insideCode = !candidateWithinCode.empty() && *candidateWithinCode.begin() != "..";
        const bool supportedExtension = candidateAbsolute.extension() == ".aup" || candidateAbsolute.extension() == ".au3";

        if (inputEmpty)
            ImGui::TextColored(ImVec4(0.92f, 0.56f, 0.46f, 1.0f), "Main file is required.");
        else if (!insideCode)
            ImGui::TextColored(ImVec4(0.92f, 0.56f, 0.46f, 1.0f), "Main file must be inside code/.");
        else if (!supportedExtension)
            ImGui::TextColored(ImVec4(0.92f, 0.56f, 0.46f, 1.0f), "Main file must use .aup or .au3.");
        else
            ImGui::TextDisabled("Resolved: %s", candidateAbsolute.string().c_str());

        ImGui::Spacing();
        const bool canApply = !inputEmpty && insideCode && supportedExtension;
        if (!canApply)
            ImGui::BeginDisabled();
        if (ImGui::Button("Apply", ImVec2(100.0f, 0.0f)))
        {
            project.mainFilePath = candidateAbsolute;
            SaveProject(project);
            dialog.mainFilePath = MakeProjectRelativePath(project, project.mainFilePath).generic_string();
            if (HasOpenDocument(state))
                CurrentDocument(state).status = "Updated project main file to " + dialog.mainFilePath;
            SaveProjectWorkspace(state);
            dialog.show = false;
        }
        if (!canApply)
            ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100.0f, 0.0f)))
            dialog.show = false;

        ImGui::End();
    }

    struct ScopedFont
    {
        explicit ScopedFont(ImFont* font) : mActive(font != nullptr)
        {
            if (mActive)
                ImGui::PushFont(font);
        }

        ~ScopedFont()
        {
            if (mActive)
                ImGui::PopFont();
        }

        bool mActive;
    };

    std::string Trim(const std::string& value)
    {
        const auto begin = value.find_first_not_of(" \t\r\n");
        if (begin == std::string::npos)
            return {};

        const auto end = value.find_last_not_of(" \t\r\n");
        return value.substr(begin, end - begin + 1U);
    }

    void DrawInlineDivider()
    {
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();
    }

    bool DrawVerticalSplitter(const char* id, float* leftWidth, float minLeftWidth, float minRightWidth, float totalWidth, float height)
    {
        constexpr float kHitWidth = 12.0f;
        constexpr float kVisibleWidth = 2.0f;
        ImGui::InvisibleButton(id, ImVec2(kHitWidth, height));
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const ImVec2 itemMin = ImGui::GetItemRectMin();
        const ImVec2 itemMax = ImGui::GetItemRectMax();
        const float lineX = itemMin.x + (kHitWidth - kVisibleWidth) * 0.5f;
        const ImU32 lineColor = ImGui::GetColorU32(ImGui::IsItemHovered() || ImGui::IsItemActive() ? ImGuiCol_SeparatorActive : ImGuiCol_Separator);
        drawList->AddRectFilled(ImVec2(lineX, itemMin.y), ImVec2(lineX + kVisibleWidth, itemMax.y), lineColor, 1.0f);

        if (ImGui::IsItemHovered() || ImGui::IsItemActive())
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

        if (!ImGui::IsItemActive())
            return false;

        *leftWidth += ImGui::GetIO().MouseDelta.x;
        *leftWidth = std::clamp(*leftWidth, minLeftWidth, totalWidth - minRightWidth);
        return true;
    }

    bool DrawHorizontalSplitter(const char* id, float* topHeight, float minTopHeight, float minBottomHeight, float totalHeight)
    {
        constexpr float kHitHeight = 12.0f;
        constexpr float kVisibleHeight = 2.0f;
        ImGui::InvisibleButton(id, ImVec2(-1.0f, kHitHeight));
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const ImVec2 itemMin = ImGui::GetItemRectMin();
        const ImVec2 itemMax = ImGui::GetItemRectMax();
        const float lineY = itemMin.y + (kHitHeight - kVisibleHeight) * 0.5f;
        const ImU32 lineColor = ImGui::GetColorU32(ImGui::IsItemHovered() || ImGui::IsItemActive() ? ImGuiCol_SeparatorActive : ImGuiCol_Separator);
        drawList->AddRectFilled(ImVec2(itemMin.x, lineY), ImVec2(itemMax.x, lineY + kVisibleHeight), lineColor, 1.0f);

        if (ImGui::IsItemHovered() || ImGui::IsItemActive())
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);

        if (!ImGui::IsItemActive())
            return false;

        *topHeight += ImGui::GetIO().MouseDelta.y;
        *topHeight = std::clamp(*topHeight, minTopHeight, totalHeight - minBottomHeight);
        return true;
    }

    void StartFileAction(
        EditorState& state,
        FileActionState::Kind kind,
        const std::filesystem::path& sourcePath,
        const std::filesystem::path& baseDirectory,
        const std::filesystem::path& suggestedPath = {},
        bool openAfterCreate = false)
    {
        state.fileAction.kind = kind;
        state.fileAction.sourcePath = sourcePath;
        state.fileAction.baseDirectory = baseDirectory;
        state.fileAction.inputPath = suggestedPath.generic_string();
        state.fileAction.openAfterCreate = openAfterCreate;
        state.fileAction.openRequested = true;
    }

    bool SubmitFileActionInput(EditorState& state, const char* title, const char* prompt)
    {
        if (!ImGui::BeginPopupModal(title, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            return false;

        const bool windowAppearing = ImGui::IsWindowAppearing();
        if (windowAppearing)
            ImGui::SetKeyboardFocusHere();

        ImGui::TextWrapped("%s", prompt);
        const bool submitted = ImGui::InputText(
            "##TargetPath",
            &state.fileAction.inputPath,
            ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue);
        if (windowAppearing && (state.fileAction.kind == FileActionState::Kind::NewFile || state.fileAction.kind == FileActionState::Kind::NewFolder))
        {
            if (ImGuiInputTextState* inputState = ImGui::GetInputTextState(ImGui::GetItemID()))
            {
                const std::string& input = state.fileAction.inputPath;
                const std::size_t separator = input.find_last_of("/\\");
                const int selectionStart = separator == std::string::npos ? 0 : static_cast<int>(separator + 1U);
                int selectionEnd = static_cast<int>(input.size());
                if (state.fileAction.kind == FileActionState::Kind::NewFile)
                {
                    const std::size_t extension = input.find_last_of('.');
                    if (extension != std::string::npos && extension > static_cast<std::size_t>(selectionStart))
                        selectionEnd = static_cast<int>(extension);
                }
                inputState->WantReloadUserBuf = true;
                inputState->ReloadSelectionStart = selectionStart;
                inputState->ReloadSelectionEnd = selectionEnd;
                inputState->CursorFollow = true;
            }
        }
        ImGui::Separator();
        return submitted;
    }

    void DrawUnsavedDialog(EditorState& state)
    {
        if (!ImGui::BeginPopupModal("Unsaved Changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            return;

        ImGui::TextUnformatted("The current document has unsaved changes.");
        ImGui::Separator();

        if (ImGui::Button("Save", ImVec2(100.0f, 0.0f)))
        {
            SaveDocument(CurrentDocument(state));
            ExecutePendingAction(state);
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Discard", ImVec2(100.0f, 0.0f)))
        {
            ExecutePendingAction(state);
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100.0f, 0.0f)))
        {
            state.pendingAction = {};
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    void SetUiStatus(EditorState& state, const std::string& message)
    {
        if (HasOpenDocument(state))
            CurrentDocument(state).status = message;
        else
            state.runStatus = message;
    }

    void DrawFileActionDialog(EditorState& state)
    {
        if (!state.project.has_value())
            return;

        if (state.fileAction.openRequested)
        {
            if (const char* popupTitle = FileActionPopupTitle(state.fileAction.kind))
                ImGui::OpenPopup(popupTitle);
            state.fileAction.openRequested = false;
        }

        auto finish = [&state]() { state.fileAction = {}; };
        auto makeAbsoluteTarget = [&state]() {
            const std::filesystem::path inputPath(state.fileAction.inputPath);
            if (inputPath.is_absolute())
                return inputPath.lexically_normal();

            if (state.fileAction.kind == FileActionState::Kind::NewFile || state.fileAction.kind == FileActionState::Kind::NewFolder)
                return (state.fileAction.baseDirectory / inputPath).lexically_normal();

            return MakeAbsoluteProjectPath(*state.project, inputPath);
        };

        if (state.fileAction.kind == FileActionState::Kind::NewFile)
        {
            const bool submittedByEnter = SubmitFileActionInput(state, "New File", "Create a new file relative to the selected folder.");
            const bool submit = ImGui::Button("Create", ImVec2(110.0f, 0.0f)) || submittedByEnter;
            if (submit)
            {
                const auto targetPath = makeAbsoluteTarget();
                if (targetPath.has_parent_path())
                    std::filesystem::create_directories(targetPath.parent_path());
                WriteTextFile(targetPath, "");
                if (state.fileAction.openAfterCreate && IsEditableProjectFile(targetPath))
                    state.requestedOpenPath = targetPath;
                state.projectTree.clear();
                state.projectTreeRoot.clear();
                SetUiStatus(state, "Created " + targetPath.string());
                finish();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(110.0f, 0.0f)))
            {
                finish();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (state.fileAction.kind == FileActionState::Kind::NewFolder)
        {
            const bool submittedByEnter = SubmitFileActionInput(state, "New Folder", "Create a new folder relative to the selected folder.");
            const bool submit = ImGui::Button("Create", ImVec2(110.0f, 0.0f)) || submittedByEnter;
            if (submit)
            {
                const auto targetPath = makeAbsoluteTarget();
                std::filesystem::create_directories(targetPath);
                state.projectTree.clear();
                state.projectTreeRoot.clear();
                SetUiStatus(state, "Created " + targetPath.string());
                finish();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(110.0f, 0.0f)))
            {
                finish();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (state.fileAction.kind == FileActionState::Kind::Rename)
        {
            const bool submittedByEnter = SubmitFileActionInput(state, "Rename", "Enter the new file or folder name.");
            const bool submit = ImGui::Button("Rename", ImVec2(110.0f, 0.0f)) || submittedByEnter;
            if (submit)
            {
                const auto targetPath = state.fileAction.sourcePath.parent_path() / state.fileAction.inputPath;
                std::filesystem::rename(state.fileAction.sourcePath, targetPath);
                if (const auto existingIndex = FindDocumentIndexByPath(state, state.fileAction.sourcePath))
                {
                    state.documents[*existingIndex].path = targetPath;
                    state.documents[*existingIndex].title = MakeDocumentTitle(targetPath);
                    state.documents[*existingIndex].previewDirty = true;
                    state.documents[*existingIndex].outlineDirty = true;
                    state.currentDocumentIndex = *existingIndex;
                    state.activateDocumentIndex = *existingIndex;
                }
                if (state.project->mainFilePath == state.fileAction.sourcePath)
                    state.project->mainFilePath = targetPath;
                if (state.selectedProjectPath == state.fileAction.sourcePath)
                    state.selectedProjectPath = targetPath;
                state.projectTree.clear();
                state.projectTreeRoot.clear();
                SaveProjectWorkspace(state);
                SetUiStatus(state, "Renamed to " + targetPath.string());
                finish();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(110.0f, 0.0f)))
            {
                finish();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (state.fileAction.kind == FileActionState::Kind::Copy)
        {
            const bool submittedByEnter = SubmitFileActionInput(state, "Copy Item", "Enter the target project-relative path for the copy.");
            const bool submit = ImGui::Button("Copy", ImVec2(110.0f, 0.0f)) || submittedByEnter;
            if (submit)
            {
                const auto targetPath = makeAbsoluteTarget();
                CopyPathRecursively(state.fileAction.sourcePath, targetPath);
                state.projectTree.clear();
                state.projectTreeRoot.clear();
                SetUiStatus(state, "Copied to " + targetPath.string());
                finish();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(110.0f, 0.0f)))
            {
                finish();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (state.fileAction.kind == FileActionState::Kind::Duplicate)
        {
            const bool submittedByEnter = SubmitFileActionInput(state, "Duplicate Item", "Choose the duplicate path relative to the project root.");
            const bool submit = ImGui::Button("Duplicate", ImVec2(110.0f, 0.0f)) || submittedByEnter;
            if (submit)
            {
                const auto targetPath = makeAbsoluteTarget();
                CopyPathRecursively(state.fileAction.sourcePath, targetPath);
                if (IsEditableProjectFile(targetPath))
                    state.requestedOpenPath = targetPath;
                state.projectTree.clear();
                state.projectTreeRoot.clear();
                SetUiStatus(state, "Duplicated to " + targetPath.string());
                finish();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(110.0f, 0.0f)))
            {
                finish();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (state.fileAction.kind == FileActionState::Kind::Delete)
        {
            if (!ImGui::BeginPopupModal("Delete Item", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
                return;

            ImGui::TextWrapped("Delete %s?", state.fileAction.sourcePath.filename().string().c_str());
            ImGui::TextDisabled("%s", MakeProjectRelativePath(*state.project, state.fileAction.sourcePath).generic_string().c_str());
            ImGui::Separator();

            if (ImGui::Button("Delete", ImVec2(110.0f, 0.0f)))
            {
                if (std::filesystem::is_directory(state.fileAction.sourcePath))
                    std::filesystem::remove_all(state.fileAction.sourcePath);
                else
                    std::filesystem::remove(state.fileAction.sourcePath);

                if (const auto existingIndex = FindDocumentIndexByPath(state, state.fileAction.sourcePath))
                    CloseDocument(state, *existingIndex);

                state.projectTree.clear();
                state.projectTreeRoot.clear();
                finish();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(110.0f, 0.0f)))
            {
                finish();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    void DrawMenuContents(EditorState& state)
    {
        ConfigureDefaultHotkeys(state);

        const auto shortcutLabel = [&](const char* id) -> const char* {
            static std::string label;
            if (const auto value = state.hotkeys.LabelFor(id))
            {
                label = *value;
                return label.c_str();
            }
            return nullptr;
        };

        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New Project..."))
            {
                if (const auto selectedPath = ShowFileDialog(true, "Torii Project\0*.torii\0All Files\0*.*\0"))
                {
                    auto projectPath = *selectedPath;
                    projectPath.replace_extension(kProjectExtension);
                    LoadProjectIntoEditor(state, CreateNewProject(projectPath));
                }
            }

            if (ImGui::MenuItem("Open Project..."))
            {
                if (const auto selectedPath = ShowFileDialog(false, "Torii Project\0*.torii\0All Files\0*.*\0"))
                    LoadProjectIntoEditor(state, LoadProject(*selectedPath));
            }

            if (state.project.has_value() && ImGui::MenuItem("Save Project"))
                SaveProject(*state.project);

            ImGui::Separator();
            const bool hasDocument = HasOpenDocument(state);

            if (ImGui::MenuItem("Save", shortcutLabel("save"), false, hasDocument))
                SaveDocument(CurrentDocument(state));

            if (ImGui::MenuItem("Save All", shortcutLabel("save_all"), false, hasDocument))
                SaveAllDocuments(state);

            if (ImGui::MenuItem("Save As...", shortcutLabel("save_as"), false, hasDocument))
                SaveDocumentAs(CurrentDocument(state));

            if (ImGui::MenuItem("Save Output", nullptr, false, hasDocument))
                SaveDerivedOutput(CurrentDocument(state));

            if (ImGui::MenuItem("Save Output As...", nullptr, false, hasDocument))
                SaveDerivedOutputAs(CurrentDocument(state));

            ImGui::Separator();

            if (ImGui::MenuItem("Exit", "Alt+F4"))
            {
                if (hasDocument && CurrentDocument(state).dirty)
                    RequestAction(state, PendingAction::Kind::ExitApplication);
                else
                    state.requestExit = true;
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit"))
        {
            const bool hasDocument = HasOpenDocument(state);
            if (ImGui::MenuItem("Undo", shortcutLabel("undo"), false, CanUndoCurrentDocument(state)))
                UndoCurrentDocument(state);

            if (ImGui::MenuItem("Redo", shortcutLabel("redo"), false, CanRedoCurrentDocument(state)))
                RedoCurrentDocument(state);

            if (ImGui::MenuItem("Show Whitespace", nullptr, state.preferences.showWhitespace, hasDocument))
            {
                state.preferences.showWhitespace = !state.preferences.showWhitespace;
                ApplyPreferences(state);
            }

            if (ImGui::MenuItem("Show Line Numbers", nullptr, state.preferences.showLineNumbers, hasDocument))
            {
                state.preferences.showLineNumbers = !state.preferences.showLineNumbers;
                ApplyPreferences(state);
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Actions"))
        {
            if (ImGui::MenuItem("Build", shortcutLabel("build"), false, state.project.has_value()))
                ExecuteUiAction(state, [&]() { BuildCurrentDocument(state); });
            if (state.project.has_value() && ImGui::MenuItem("Run", shortcutLabel("run")))
                ExecuteUiAction(state, [&]() { RunBuiltProject(state); });
            if (state.project.has_value())
            {
                ImGui::Separator();
                if (ImGui::MenuItem("Debug", nullptr, state.buildConfiguration == BuildConfiguration::Debug))
                {
                    state.buildConfiguration = BuildConfiguration::Debug;
                    SaveProjectWorkspace(state);
                }
                if (ImGui::MenuItem("Release", nullptr, state.buildConfiguration == BuildConfiguration::Release))
                {
                    state.buildConfiguration = BuildConfiguration::Release;
                    SaveProjectWorkspace(state);
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Project", nullptr, &state.showInspector);
            ImGui::MenuItem("Symbols", nullptr, &state.showOutline);
            ImGui::MenuItem("Output", nullptr, &state.showOutput);
            ImGui::MenuItem("Run", nullptr, &state.showRun);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Options"))
        {
            if (ImGui::MenuItem("Preferences"))
                state.preferences.showPreferences = true;
            ImGui::EndMenu();
        }
    }

    void HandleShortcuts(EditorState& state)
    {
        ConfigureDefaultHotkeys(state);
        state.hotkeys.Process(state);
    }

    void ConfigureDefaultHotkeys(EditorState& state)
    {
        if (state.hotkeys.Find("save") != nullptr)
            return;

        const auto resolvedChord = [&](const char* id, HotkeyChord fallback) {
            const auto it = state.shortcutOverrides.find(id);
            if (it != state.shortcutOverrides.end())
            {
                if (const auto parsed = HotkeyManager::ParseChord(it->second))
                    return *parsed;
            }
            return fallback;
        };

        const auto registerBinding = [&](const char* id, HotkeyChord fallback, HotkeyManager::Callback callback, HotkeyManager::Predicate predicate) {
            const HotkeyChord chord = resolvedChord(id, fallback);
            state.hotkeys.Register({
                id,
                chord,
                HotkeyManager::FormatChord(chord),
                std::move(callback),
                std::move(predicate)
            });
        };

        registerBinding(
            "save_all",
            HotkeyChord{ImGuiKey_S, true, true, false, false},
            [](EditorState& editorState) { SaveAllDocuments(editorState); },
            [](const EditorState& editorState) { return HasOpenDocument(editorState); });

        registerBinding(
            "save_as",
            HotkeyChord{ImGuiKey_S, true, false, true, false},
            [](EditorState& editorState) { SaveDocumentAs(CurrentDocument(editorState)); },
            [](const EditorState& editorState) { return HasOpenDocument(editorState); });

        registerBinding(
            "save",
            HotkeyChord{ImGuiKey_S, true, false, false, false},
            [](EditorState& editorState) { SaveDocument(CurrentDocument(editorState)); },
            [](const EditorState& editorState) { return HasOpenDocument(editorState); });

        registerBinding(
            "undo",
            HotkeyChord{ImGuiKey_Z, true, false, false, false},
            [](EditorState& editorState) { UndoCurrentDocument(editorState); },
            [](const EditorState& editorState) { return CanUndoCurrentDocument(editorState); });

        registerBinding(
            "redo",
            HotkeyChord{ImGuiKey_Y, true, false, false, false},
            [](EditorState& editorState) { RedoCurrentDocument(editorState); },
            [](const EditorState& editorState) { return CanRedoCurrentDocument(editorState); });

        registerBinding(
            "build",
            HotkeyChord{ImGuiKey_B, true, false, false, false},
            [](EditorState& editorState) { ExecuteUiAction(editorState, [&]() { BuildCurrentDocument(editorState); }); },
            [](const EditorState& editorState) { return editorState.project.has_value(); });

        registerBinding(
            "run",
            HotkeyChord{ImGuiKey_F5, false, false, false, false},
            [](EditorState& editorState) { ExecuteUiAction(editorState, [&]() { RunBuiltProject(editorState); }); },
            [](const EditorState& editorState) { return editorState.project.has_value(); });

        registerBinding(
            "rename_project_item",
            HotkeyChord{ImGuiKey_F2, false, false, false, false},
            [](EditorState& editorState) {
                if (!editorState.selectedProjectPath.has_value() || !editorState.project.has_value())
                    return;
                const auto selectedPath = *editorState.selectedProjectPath;
                if (selectedPath == editorState.project->rootDirectory)
                    return;
                StartFileAction(editorState, FileActionState::Kind::Rename, selectedPath, selectedPath.parent_path(), selectedPath.filename());
            },
            [](const EditorState& editorState) {
                return editorState.selectedProjectPath.has_value()
                    && editorState.project.has_value()
                    && *editorState.selectedProjectPath != editorState.project->rootDirectory;
            });

        registerBinding(
            "zoom_in",
            HotkeyChord{ImGuiKey_Equal, true, true, false, false},
            [](EditorState& editorState) { AdjustGlobalZoom(editorState, 0.10f); },
            [](const EditorState&) { return true; });

        registerBinding(
            "zoom_out",
            HotkeyChord{ImGuiKey_Minus, true, false, false, false},
            [](EditorState& editorState) { AdjustGlobalZoom(editorState, -0.10f); },
            [](const EditorState&) { return true; });

        state.shortcutOverrides.clear();
        for (const auto& [id, chord] : state.hotkeys.ExportBindings())
            state.shortcutOverrides[id] = chord;
        SyncShortcutEditBuffers(state);
    }

    void DrawTabs(EditorState& state)
    {
        if (!ImGui::BeginTabBar("Documents", ImGuiTabBarFlags_Reorderable))
            return;

        std::optional<std::size_t> clickedIndex;
        std::optional<std::size_t> selectedIndex;
        enum class TabContextAction
        {
            None,
            CloseThis,
            CloseOthers,
            CloseRight
        };
        std::optional<std::pair<TabContextAction, std::size_t>> pendingAction;
        for (std::size_t index = 0; index < state.documents.size(); ++index)
        {
            auto& document = state.documents[index];
            bool keepOpen = true;
            const std::string displayLabel = document.dirty ? (document.title + "*") : document.title;
            const std::string label = displayLabel + "###DocumentTab" + std::to_string(index);
            ImGuiTabItemFlags flags = ImGuiTabItemFlags_None;
            if (state.activateDocumentIndex.has_value() && *state.activateDocumentIndex == index)
                flags |= ImGuiTabItemFlags_SetSelected;

            if (ImGui::BeginTabItem(label.c_str(), &keepOpen, flags))
            {
                selectedIndex = index;
                if (ImGui::IsItemClicked())
                    clickedIndex = index;

                const std::string popupId = "##DocumentTabContext" + std::to_string(index);
                if (ImGui::BeginPopupContextItem(popupId.c_str()))
                {
                    if (ImGui::MenuItem("Close This"))
                        pendingAction = {std::make_pair(TabContextAction::CloseThis, index)};
                    if (ImGui::MenuItem("Close Others", nullptr, false, state.documents.size() > 1U))
                        pendingAction = {std::make_pair(TabContextAction::CloseOthers, index)};
                    if (ImGui::MenuItem("Close To The Right", nullptr, false, index + 1U < state.documents.size()))
                        pendingAction = {std::make_pair(TabContextAction::CloseRight, index)};
                    ImGui::EndPopup();
                }
                ImGui::EndTabItem();
            }
            else
            {
                const std::string popupId = "##DocumentTabContext" + std::to_string(index);
                if (ImGui::BeginPopupContextItem(popupId.c_str()))
                {
                    if (ImGui::MenuItem("Close This"))
                        pendingAction = {std::make_pair(TabContextAction::CloseThis, index)};
                    if (ImGui::MenuItem("Close Others", nullptr, false, state.documents.size() > 1U))
                        pendingAction = {std::make_pair(TabContextAction::CloseOthers, index)};
                    if (ImGui::MenuItem("Close To The Right", nullptr, false, index + 1U < state.documents.size()))
                        pendingAction = {std::make_pair(TabContextAction::CloseRight, index)};
                    ImGui::EndPopup();
                }
            }

            if (!keepOpen)
            {
                pendingAction = {std::make_pair(TabContextAction::CloseThis, index)};
                break;
            }
        }

        ImGui::EndTabBar();
        if (selectedIndex.has_value())
            state.currentDocumentIndex = *selectedIndex;
        if (clickedIndex.has_value())
        {
            state.currentDocumentIndex = *clickedIndex;
            FocusCurrentEditor(state);
        }
        if (pendingAction.has_value())
        {
            const auto [action, index] = *pendingAction;
            switch (action)
            {
                case TabContextAction::CloseThis:
                    CloseDocumentWithPrompt(state, index);
                    break;
                case TabContextAction::CloseOthers:
                    CloseOtherTabs(state, index);
                    break;
                case TabContextAction::CloseRight:
                    CloseTabsToRight(state, index);
                    break;
                case TabContextAction::None:
                    break;
            }
        }
        state.activateDocumentIndex.reset();
    }

    void DrawProjectTreeNode(EditorState& state, const ProjectTreeNode& node, int depth)
    {
        auto toggleDirectoryExpansion = [&](const std::filesystem::path& directoryPath, bool expanded) {
            const auto key = std::filesystem::absolute(directoryPath).lexically_normal().generic_string();
            if (expanded)
                state.expandedProjectDirectories.insert(key);
            else
                state.expandedProjectDirectories.erase(key);
            SaveProjectWorkspace(state);
        };

        auto handleProjectTreeDrop = [&](const std::filesystem::path& dropTargetDirectory, bool /*directoryStyle*/) {
            if (!ImGui::BeginDragDropTarget())
                return;

            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(
                "PROJECT_PATH",
                ImGuiDragDropFlags_AcceptNoDrawDefaultRect | ImGuiDragDropFlags_AcceptBeforeDelivery))
            {
                if (payload->IsPreview())
                {
                    state.projectTreeDropPreviewDirectory = std::filesystem::absolute(dropTargetDirectory).lexically_normal();
                }

                if (!payload->IsDelivery())
                {
                    ImGui::EndDragDropTarget();
                    return;
                }

                const auto* rawPath = static_cast<const char*>(payload->Data);
                std::filesystem::path sourcePath(rawPath == nullptr ? "" : rawPath);
                sourcePath = std::filesystem::absolute(sourcePath).lexically_normal();
                const auto normalizedTargetDirectory = std::filesystem::absolute(dropTargetDirectory).lexically_normal();

                try
                {
                    if (!std::filesystem::exists(sourcePath))
                        throw std::runtime_error("Dragged path does not exist.");
                    if (sourcePath == normalizedTargetDirectory)
                        throw std::runtime_error("Cannot move an item onto itself.");

                    if (std::filesystem::is_directory(sourcePath))
                    {
                        const auto sourceString = sourcePath.generic_string();
                        const auto targetString = normalizedTargetDirectory.generic_string();
                        if (targetString == sourceString || (targetString.size() > sourceString.size()
                            && targetString.compare(0, sourceString.size(), sourceString) == 0
                            && targetString[sourceString.size()] == '/'))
                        {
                            throw std::runtime_error("Cannot move a folder into itself.");
                        }
                    }

                    MoveProjectPath(state, sourcePath, normalizedTargetDirectory);
                    state.selectedProjectPath = normalizedTargetDirectory / sourcePath.filename();
                    SaveProject(*state.project);
                    SaveProjectWorkspace(state);
                    RequestProjectTreeRefresh(state);
                    SetUiStatus(state, "Moved " + sourcePath.string() + " to " + normalizedTargetDirectory.string());
                }
                catch (const std::exception& exception)
                {
                    SetUiStatus(state, exception.what());
                    state.outputLog += "[ERROR] " + std::string(exception.what()) + "\n";
                    if (state.outputEditor != nullptr)
                        SetLoggerText(*state.outputEditor, state.outputLog);
                }
            }

            ImGui::EndDragDropTarget();
        };

        const auto& path = node.path;
        const bool isDirectory = node.isDirectory;
        const auto& name = node.name;
        const bool isSelected = state.selectedProjectPath.has_value() && *state.selectedProjectPath == path;
        const bool isBuildTarget = state.project.has_value()
            && std::filesystem::absolute(state.project->mainFilePath).lexically_normal() == std::filesystem::absolute(path).lexically_normal();

        if (isDirectory)
        {
            const auto directoryKey = std::filesystem::absolute(path).lexically_normal().generic_string();
            const bool expanded = state.expandedProjectDirectories.contains(directoryKey);
            const FileTreeRowResult row = DrawFileTreeRow(
                path.string(),
                name,
                depth,
                isSelected,
                true,
                expanded,
                "folder-fill",
                "folder-opened",
                ActiveTheme(state.preferences).uiTheme.iconSecondary);
            if (row.clicked)
                state.selectedProjectPath = path;
            if (row.toggleOpen)
                toggleDirectoryExpansion(path, row.open);

            const ImVec2 rowMin = ImGui::GetItemRectMin();
            const ImVec2 rowMax = ImGui::GetItemRectMax();

            if (ImGui::BeginDragDropSource())
            {
                const auto payload = path.string();
                ImGui::SetDragDropPayload("PROJECT_PATH", payload.c_str(), payload.size() + 1U);
                ImGui::TextUnformatted(name.c_str());
                ImGui::EndDragDropSource();
            }

            handleProjectTreeDrop(path, true);

            const std::string dirContextId = "##ProjectTreeDirContext" + path.string();
            if (ImGui::BeginPopupContextItem(dirContextId.c_str()))
            {
                if (ImGui::MenuItem("New File"))
                    StartFileAction(state, FileActionState::Kind::NewFile, {}, path, "NewFile.aup", true);
                if (ImGui::MenuItem("New Folder"))
                    StartFileAction(state, FileActionState::Kind::NewFolder, {}, path, "NewFolder");
                if (path != state.project->rootDirectory && ImGui::MenuItem("Rename (F2)"))
                    StartFileAction(state, FileActionState::Kind::Rename, path, path.parent_path(), path.filename());
                if (path != state.project->rootDirectory && ImGui::MenuItem("Copy..."))
                    StartFileAction(state, FileActionState::Kind::Copy, path, path.parent_path(), MakeProjectRelativePath(*state.project, path));
                if (path != state.project->rootDirectory && ImGui::MenuItem("Duplicate"))
                    StartFileAction(state, FileActionState::Kind::Duplicate, path, path.parent_path(), MakeProjectRelativePath(*state.project, EnsureUniquePath(path)));
                if (path != state.project->rootDirectory && ImGui::MenuItem("Delete"))
                    StartFileAction(state, FileActionState::Kind::Delete, path, path.parent_path());
                ImGui::EndPopup();
            }

            if (row.open)
            {
                for (const auto& child : node.children)
                    DrawProjectTreeNode(state, child, depth + 1);
            }

            const ImVec2 windowPos = ImGui::GetWindowPos();
            const ImVec2 localContentMin = ImGui::GetWindowContentRegionMin();
            const ImVec2 localContentMax = ImGui::GetWindowContentRegionMax();
            const ImVec2 contentMin(windowPos.x + localContentMin.x, windowPos.y + localContentMin.y);
            const ImVec2 contentMax(windowPos.x + localContentMax.x, windowPos.y + localContentMax.y);
            constexpr float kTreeIndentWidth = 18.0f;
            constexpr float kTreeRowPaddingX = 6.0f;
            const auto scopeKey = std::filesystem::absolute(path).lexically_normal().generic_string();
            const bool hasVisibleChildren = row.open && !node.children.empty();
            const float contentIndentX = contentMin.x + kTreeRowPaddingX + (depth + 1) * kTreeIndentWidth - 2.0f;
            const float rowIndentX = contentMin.x + kTreeRowPaddingX + depth * kTreeIndentWidth - 2.0f;
            state.projectTreeScopeRects[scopeKey] = ProjectTreeScopeRect{
                .min = ImVec2(hasVisibleChildren ? contentIndentX : rowIndentX, hasVisibleChildren ? rowMax.y - 1.0f : rowMin.y - 2.0f),
                .max = ImVec2(contentMax.x - 4.0f, hasVisibleChildren ? std::max(rowMax.y + 8.0f, ImGui::GetCursorScreenPos().y) + 2.0f : rowMax.y + 2.0f)
            };
            return;
        }

        const FileTreeRowResult row = DrawFileTreeRow(
            path.string(),
            name,
            depth,
            isSelected,
            false,
            false,
            FindKnownFileTypeIcon(path),
            FindKnownFileTypeIcon(path),
            isBuildTarget ? ActiveTheme(state.preferences).uiTheme.iconSuccess : ActiveTheme(state.preferences).uiTheme.iconPrimary,
            isBuildTarget ? "[Build]" : nullptr,
            isBuildTarget ? &ActiveTheme(state.preferences).uiTheme.iconSuccess : nullptr);
        if (row.clicked)
            state.selectedProjectPath = path;
        if (row.doubleClicked && IsEditableProjectFile(path))
            state.requestedOpenPath = path;

        if (ImGui::BeginDragDropSource())
        {
            const auto payload = path.string();
            ImGui::SetDragDropPayload("PROJECT_PATH", payload.c_str(), payload.size() + 1U);
            ImGui::TextUnformatted(name.c_str());
            ImGui::EndDragDropSource();
        }

        handleProjectTreeDrop(path.parent_path(), false);

        const std::string fileContextId = "##ProjectTreeFileContext" + path.string();
        if (ImGui::BeginPopupContextItem(fileContextId.c_str()))
        {
            if (IsEditableProjectFile(path) && ImGui::MenuItem("Open"))
                state.requestedOpenPath = path;
            if (IsEditableProjectFile(path)
                && (!state.project.has_value() || std::filesystem::absolute(state.project->mainFilePath).lexically_normal() != std::filesystem::absolute(path).lexically_normal())
                && ImGui::MenuItem("Set As Build Target"))
            {
                state.project->mainFilePath = std::filesystem::absolute(path).lexically_normal();
                SaveProject(*state.project);
                SaveProjectWorkspace(state);
                SetUiStatus(state, "Updated build target to " + MakeProjectRelativePath(*state.project, state.project->mainFilePath).generic_string());
            }
            if (ImGui::MenuItem("Rename (F2)"))
                StartFileAction(state, FileActionState::Kind::Rename, path, path.parent_path(), path.filename());
            if (ImGui::MenuItem("Copy..."))
                StartFileAction(state, FileActionState::Kind::Copy, path, path.parent_path(), MakeProjectRelativePath(*state.project, path));
            if (ImGui::MenuItem("Duplicate"))
                StartFileAction(state, FileActionState::Kind::Duplicate, path, path.parent_path(), MakeProjectRelativePath(*state.project, EnsureUniquePath(path)));
            if (ImGui::MenuItem("Delete"))
                StartFileAction(state, FileActionState::Kind::Delete, path, path.parent_path());
            ImGui::EndPopup();
        }
    }

    void DrawSidebar(EditorState& state, const ImVec2& size = ImVec2(0.0f, 0.0f))
    {
        const auto& theme = ActiveTheme(state.preferences);
        const auto& icons = theme.uiTheme;
        ImGui::PushStyleColor(ImGuiCol_ChildBg, theme.uiTheme.panelColor);
        ImGui::BeginChild("Sidebar", size, true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        {
            ScopedFont sidebarFont(ChooseMonoFont(1.0f));

            if (state.project.has_value())
            {
                PollProjectTreeRefresh(state);
                DrawSectionHeader("window-sidebar", state.project->name.c_str(), nullptr, icons.iconPrimary);
                ImGui::TextDisabled("%s", state.project->rootDirectory.string().c_str());
                ImGui::Spacing();

                if (DrawToolbarButton("SidebarNewFile", "new-file", "", true, icons.iconPrimary))
                    StartFileAction(state, FileActionState::Kind::NewFile, {}, state.project->rootDirectory / "code", "NewFile.aup", true);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("New File");
                ImGui::SameLine();
                if (DrawToolbarButton("SidebarNewFolder", "new-folder", "", true, icons.iconSecondary))
                    StartFileAction(state, FileActionState::Kind::NewFolder, {}, state.project->rootDirectory / "code", "NewFolder");
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("New Folder");
                ImGui::SameLine();
                if (DrawToolbarButton("SidebarProjectSettings", "gear-fill", "", true, icons.iconNeutral))
                    OpenProjectSettings(state);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Project Settings");

                ImGui::Separator();
                DrawSectionHeader("root-folder-opened", "Code", nullptr, icons.iconSecondary);
                const auto sourceRoot = std::filesystem::exists(state.project->rootDirectory / "code")
                    ? state.project->rootDirectory / "code"
                    : state.project->rootDirectory;
                if (state.projectTreeRoot != sourceRoot && !state.projectTreeLoading)
                {
                    state.projectTree.clear();
                    RequestProjectTreeRefresh(state);
                }

                if (state.projectTreeLoading)
                    ImGui::TextDisabled("Loading project tree...");

                state.projectTreeDropPreviewDirectory.reset();
                state.projectTreeScopeRects.clear();
                const ImVec2 treeStart = ImGui::GetCursorScreenPos();
                for (const auto& entry : state.projectTree)
                    DrawProjectTreeNode(state, entry, 0);
                const ImVec2 treeEnd = ImGui::GetCursorScreenPos();
                if (state.project.has_value())
                {
                    const auto sourceRootKey = std::filesystem::absolute(sourceRoot).lexically_normal().generic_string();
                    const ImVec2 windowPos = ImGui::GetWindowPos();
                    const ImVec2 localContentMin = ImGui::GetWindowContentRegionMin();
                    const ImVec2 localContentMax = ImGui::GetWindowContentRegionMax();
                    const ImVec2 contentMin(windowPos.x + localContentMin.x, windowPos.y + localContentMin.y);
                    const ImVec2 contentMax(windowPos.x + localContentMax.x, windowPos.y + localContentMax.y);
                    state.projectTreeScopeRects[sourceRootKey] = ProjectTreeScopeRect{
                        .min = ImVec2(contentMin.x + 4.0f, treeStart.y - 2.0f),
                        .max = ImVec2(contentMax.x - 4.0f, std::max(treeStart.y + 24.0f, treeEnd.y) + 2.0f)
                    };
                }

                if (state.projectTreeDropPreviewDirectory.has_value())
                {
                    const auto previewKey = std::filesystem::absolute(*state.projectTreeDropPreviewDirectory).lexically_normal().generic_string();
                    if (const auto scopeIt = state.projectTreeScopeRects.find(previewKey); scopeIt != state.projectTreeScopeRects.end())
                    {
                        ImDrawList* drawList = ImGui::GetForegroundDrawList();
                        auto scope = scopeIt->second;
                        if (scope.max.y < scope.min.y)
                            scope.max.y = scope.min.y + 22.0f;
                        const ImVec4 fillColorVec = state.preferences.uiTheme.directoryDropHighlight;
                        const ImU32 fillColor = ImGui::ColorConvertFloat4ToU32(ImVec4(fillColorVec.x, fillColorVec.y, fillColorVec.z, std::max(0.22f, fillColorVec.w)));
                        const ImU32 lineColor = ImGui::ColorConvertFloat4ToU32(state.preferences.uiTheme.fileDropLine);
                        drawList->AddRectFilled(scope.min, scope.max, fillColor, 8.0f);
                        drawList->AddRect(scope.min, scope.max, lineColor, 8.0f, 0, 2.0f);
                    }
                }
            }
            else
            {
                DrawSectionHeader("window-sidebar", "Torii", nullptr, icons.iconPrimary);
                ImGui::TextDisabled("No project loaded");
                ImGui::Spacing();

                if (DrawToolbarButton("SidebarNewProject", "project", "New Project", true, icons.iconPrimary))
                {
                    if (const auto selectedPath = ShowFileDialog(true, "Torii Project\0*.torii\0All Files\0*.*\0"))
                    {
                        auto projectPath = *selectedPath;
                        projectPath.replace_extension(kProjectExtension);
                        LoadProjectIntoEditor(state, CreateNewProject(projectPath));
                    }
                }

                if (DrawToolbarButton("SidebarOpenProject", "folder-opened", "Open Project", true, icons.iconSecondary))
                {
                    if (const auto selectedPath = ShowFileDialog(false, "Torii Project\0*.torii\0All Files\0*.*\0"))
                        LoadProjectIntoEditor(state, LoadProject(*selectedPath));
                }
            }
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

    void DrawOutlinePanel(EditorState& state, const ImVec2& size = ImVec2(0.0f, 0.0f))
    {
        const auto& theme = ActiveTheme(state.preferences);
        const auto& icons = theme.uiTheme;
        if (!HasOpenDocument(state))
        {
            ImGui::PushStyleColor(ImGuiCol_ChildBg, theme.uiTheme.panelColor);
            ImGui::BeginChild("OutlinePanel", size, true);
            {
                ScopedFont outlineFont(ChooseMonoFont(1.0f));
                DrawSectionHeader("symbol-misc", "Symbols", nullptr, icons.iconNeutral);
                ImGui::TextDisabled("Open a project file to inspect symbols");
            }
            ImGui::EndChild();
            ImGui::PopStyleColor();
            return;
        }

        auto& document = CurrentDocument(state);
        if (!document.dirty && (ImGui::GetTime() - document.lastEditTime) > 0.30)
            RefreshOutline(state, document);

        ImGui::PushStyleColor(ImGuiCol_ChildBg, theme.uiTheme.panelColor);
        ImGui::BeginChild("OutlinePanel", size, true);
        {
            ScopedFont outlineFont(ChooseMonoFont(1.0f));
            const auto outlineSubtitle = document.path.filename().string();
            DrawSectionHeader("symbol-misc", "Symbols", &outlineSubtitle, icons.iconNeutral);
            ImGui::Separator();
            if (document.outlinePending)
                ImGui::TextDisabled("Refreshing symbols...");
            auto drawSymbolLeaf = [&](const std::string& id, const std::string& label, int depth, const char* iconName, const ImVec4& color, int line, std::function<void(const FileTreeRowResult&)> onActivate = {}) {
                const FileTreeRowResult row = DrawFileTreeRow(id, label, depth, false, false, false, iconName, iconName, color);
                if (row.clicked || row.doubleClicked)
                    JumpToLine(document, line);
                if (onActivate)
                    onActivate(row);
            };

            if (!document.outline.functions.empty() && DrawTreeSectionRow("OutlineFunctions", "Functions", 0, "braces-asterisk", "braces-asterisk", icons.iconSecondary))
            {
                for (const auto& function : document.outline.functions)
                {
                    const FileTreeRowResult functionRow = DrawFileTreeRow(
                        "OutlineFunction:" + function.name + ":" + std::to_string(function.line),
                        function.name,
                        1,
                        false,
                        true,
                        false,
                        "symbol-method-arrow",
                        "symbol-method-arrow",
                        icons.iconSecondary);
                    if (functionRow.clicked)
                        JumpToLine(document, function.line);

                    if (functionRow.open)
                    {
                        if (!function.parameters.empty() && DrawTreeSectionRow(
                            "OutlineFunctionParams:" + function.name + ":" + std::to_string(function.line),
                            "Parameters",
                            2,
                            "group-by-ref-type",
                            "group-by-ref-type",
                            icons.iconSuccess,
                            true))
                        {
                            for (const auto& parameter : function.parameters)
                            {
                                drawSymbolLeaf(
                                    "OutlineParameter:" + function.name + ":" + parameter.name + ":" + std::to_string(parameter.line),
                                    parameter.name,
                                    3,
                                    "symbol-parameter",
                                    icons.iconSuccess,
                                    parameter.line);
                            }
                        }

                        if (!function.locals.empty() && DrawTreeSectionRow(
                            "OutlineFunctionLocals:" + function.name + ":" + std::to_string(function.line),
                            "Locals",
                            2,
                            "symbol-namespace",
                            "symbol-namespace",
                            icons.iconPrimary,
                            true))
                        {
                            for (const auto& local : function.locals)
                            {
                                drawSymbolLeaf(
                                    "OutlineLocal:" + function.name + ":" + local.name + ":" + std::to_string(local.line),
                                    local.name,
                                    3,
                                    "symbol-variable",
                                    icons.iconPrimary,
                                    local.line);
                            }
                        }
                    }
                }
            }

            if (!document.outline.globals.empty() && DrawTreeSectionRow("OutlineGlobals", "Globals", 0, "globe", "globe", icons.iconPrimary))
            {
                for (const auto& global : document.outline.globals)
                {
                    drawSymbolLeaf(
                        "OutlineGlobal:" + global.name + ":" + std::to_string(global.line),
                        global.name,
                        1,
                        "symbol-variable",
                        icons.iconPrimary,
                        global.line);
                }
            }

            if (!document.outline.constants.empty() && DrawTreeSectionRow("OutlineConstants", "Constants", 0, "star-fill", "star-fill", icons.iconNeutral))
            {
                for (const auto& constant : document.outline.constants)
                {
                    drawSymbolLeaf(
                        "OutlineConstant:" + constant.name + ":" + std::to_string(constant.line),
                        constant.name,
                        1,
                        "symbol-constant",
                        icons.iconNeutral,
                        constant.line);
                }
            }

            if (!document.outline.includes.empty() && DrawTreeSectionRow("OutlineIncludes", "Includes", 0, "journal-code", "journal-code", icons.iconSecondary))
            {
                for (const auto& include : document.outline.includes)
                {
                    drawSymbolLeaf(
                        "OutlineInclude:" + include.path + ":" + std::to_string(include.line),
                        include.path,
                        1,
                        include.isSystem ? "hashtag" : "journal",
                        include.isSystem ? icons.iconNeutral : icons.iconSecondary,
                        include.line,
                        [&](const FileTreeRowResult& row) {
                            if (row.doubleClicked)
                            {
                                if (const auto resolved = ResolveIncludePathForDocument(state, document, include))
                                    state.requestedOpenPath = *resolved;
                            }
                        });
                }
            }
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

    void DrawTopToolbar(EditorState& state)
    {
        if (!HasOpenDocument(state))
            return;

        const auto& theme = ActiveTheme(state.preferences);
        const auto& icons = theme.uiTheme;
        auto& document = CurrentDocument(state);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, theme.uiTheme.panelAltColor);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 5.0f));
        ImGui::BeginChild("TopToolbar", ImVec2(0.0f, 36.0f), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        {
            ScopedFont toolbarFont(ChooseUiFont(1.0f));

            if (DrawToolbarButton("Open", "folder-opened", "", true, icons.iconSecondary))
            {
                ExecuteUiAction(state, [&]() {
                    if (const auto selectedPath = ShowFileDialog(false, "Torii Files\0*.aup;*.au3\0All Files\0*.*\0", document.path))
                        LoadDocumentFromPath(document, *selectedPath, state.preferences);
                });
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Open");
            ImGui::SameLine();
            if (DrawToolbarButton("Save", "save", "", true, icons.iconSuccess))
                ExecuteUiAction(state, [&]() { SaveDocument(document); });
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Save");
            ImGui::SameLine();
            if (DrawToolbarButton("Undo", "arrow-left-circle-fill", "Undo", document.editor->CanUndo(), icons.iconNeutral))
                document.editor->Undo();
            ImGui::SameLine();
            if (DrawToolbarButton("Redo", "arrow-right-circle-fill", "Redo", document.editor->CanRedo(), icons.iconNeutral))
                document.editor->Redo();
            DrawInlineDivider();
            if (DrawToolbarButton("Build", "build", "Build", true, icons.iconPrimary))
                ExecuteUiAction(state, [&]() { BuildCurrentDocument(state); });
            if (state.project.has_value())
            {
                ImGui::SameLine();
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FramePadding.x, 6.0f));
                ImGui::SetNextItemWidth(110.0f);
                const char* buildMode = BuildConfigurationLabel(state.buildConfiguration);
                if (ImGui::BeginCombo("##BuildConfiguration", buildMode))
                {
                    const bool debugSelected = state.buildConfiguration == BuildConfiguration::Debug;
                    if (ImGui::Selectable("Debug", debugSelected))
                    {
                        state.buildConfiguration = BuildConfiguration::Debug;
                        SaveProjectWorkspace(state);
                    }
                    if (debugSelected)
                        ImGui::SetItemDefaultFocus();

                    const bool releaseSelected = state.buildConfiguration == BuildConfiguration::Release;
                    if (ImGui::Selectable("Release", releaseSelected))
                    {
                        state.buildConfiguration = BuildConfiguration::Release;
                        SaveProjectWorkspace(state);
                    }
                    if (releaseSelected)
                        ImGui::SetItemDefaultFocus();
                    ImGui::EndCombo();
                }
                ImGui::PopStyleVar();
                ImGui::SameLine();
                const ImVec4 runButton = ImVec4(
                    std::clamp(icons.iconSuccess.x * 0.62f, 0.0f, 1.0f),
                    std::clamp(icons.iconSuccess.y * 0.62f, 0.0f, 1.0f),
                    std::clamp(icons.iconSuccess.z * 0.62f, 0.0f, 1.0f),
                    1.0f);
                const ImVec4 runButtonHovered = ImVec4(
                    std::clamp(icons.iconSuccess.x * 0.78f, 0.0f, 1.0f),
                    std::clamp(icons.iconSuccess.y * 0.78f, 0.0f, 1.0f),
                    std::clamp(icons.iconSuccess.z * 0.78f, 0.0f, 1.0f),
                    1.0f);
                const ImVec4 runButtonActive = icons.iconSuccess;
                if (DrawToolbarButton("Run", "run-all", "Run", true, ImVec4(0.98f, 0.96f, 0.95f, 1.0f), &runButton, &runButtonHovered, &runButtonActive))
                    ExecuteUiAction(state, [&]() { RunBuiltProject(state); });
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Run");
            }
            DrawInlineDivider();
            if (DrawToolbarButton("ZoomOut", "zoom-out", "", true, icons.iconNeutral))
                AdjustGlobalZoom(state, -0.10f);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Zoom Out");
            ImGui::SameLine();
            if (DrawToolbarButton("ZoomIn", "zoom-in", "", true, icons.iconNeutral))
                AdjustGlobalZoom(state, 0.10f);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Zoom In");
            ImGui::SameLine();
            if (DrawToolbarButton("Preferences", "gear-fill", "", true, icons.iconSecondary))
                state.preferences.showPreferences = true;
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Preferences");
            ImGui::SameLine();
            if (ImGui::Checkbox("Whitespace", &state.preferences.showWhitespace))
                ApplyPreferences(state);
        }

        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();
    }

    void DrawWorkspace(EditorState& state, const ImVec2& size)
    {
        const auto& theme = ActiveTheme(state.preferences);
        const auto& icons = theme.uiTheme;
        ImGui::BeginChild("WorkspacePane", size, false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        if (!HasOpenDocument(state))
        {
            ImGui::PushStyleColor(ImGuiCol_ChildBg, theme.uiTheme.panelAltColor);
            ImGui::BeginChild("EmptyWorkspace", ImVec2(0.0f, 0.0f), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            {
                ScopedFont emptyFont(ChooseUiFont(1.0f));
                if (state.project.has_value())
                {
                    DrawSectionHeader("file-code", "No file open", nullptr, icons.iconPrimary);
                    ImGui::TextDisabled("Double-click a file in Code to open it.");
                }
                else
                {
                    DrawSectionHeader("window-sidebar", "No project loaded", nullptr, icons.iconPrimary);
                    ImGui::TextDisabled("Create or open a project to begin.");
                    ImGui::Spacing();
                    const Widgets::SocialLinkButton aboutLinks[] = {
                        {"ProjectRepo", "github", "Open AutoIt-Lexer on GitHub", "https://github.com/Zvendson/AutoIt-Lexer", icons.iconNeutral},
                        {"ProjectIssues", "globe", "Open the project page", "https://github.com/Zvendson/AutoIt-Lexer", icons.iconPrimary}
                    };
                    Widgets::DrawSocialIconRow(
                        aboutLinks,
                        std::size(aboutLinks),
                        [](const char* iconName, float size, const ImVec4& color) {
                            DrawIcon(iconName, size, color);
                        });
                }
            }
            ImGui::EndChild();
            ImGui::PopStyleColor();
            ImGui::EndChild();
            return;
        }

        DrawTopToolbar(state);
        ImGui::Separator();

        const float totalWidth = ImGui::GetContentRegionAvail().x;
        const float availableHeight = ImGui::GetContentRegionAvail().y;
        const float minSymbolsWidth = state.showOutline ? 180.0f : 0.0f;
        const float minEditorWidth = 280.0f;
        const float minPreviewWidth = 260.0f;
        const float splitterWidth = 12.0f;
        const float previewReserve = state.previewWidth + splitterWidth;
        const float symbolsReserve = state.showOutline ? state.outlineWidth + splitterWidth : 0.0f;
        const float maxEditorWidth = std::max(minEditorWidth, totalWidth - minPreviewWidth - minSymbolsWidth - splitterWidth * 2.0f);
        float editorWidth = std::clamp(totalWidth - previewReserve - symbolsReserve, minEditorWidth, maxEditorWidth);

        if (state.showOutline)
        {
            const float maxSymbolsWidth = std::max(minSymbolsWidth, totalWidth - editorWidth - minPreviewWidth - splitterWidth * 2.0f);
            state.outlineWidth = std::clamp(state.outlineWidth, minSymbolsWidth, maxSymbolsWidth);
            DrawOutlinePanel(state, ImVec2(state.outlineWidth, 0.0f));
            ImGui::SameLine();
            DrawVerticalSplitter("##WorkspaceOutlineSplitter", &state.outlineWidth, minSymbolsWidth, minEditorWidth + minPreviewWidth + splitterWidth, totalWidth, availableHeight);
            ImGui::SameLine();
        }

        ImGui::BeginChild("SourcePane", ImVec2(editorWidth, 0.0f), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        bool sourceHovered = false;
        {
            ScopedFont sourcePaneFont(ChooseMonoFont(1.0f));
            DrawTabs(state);
            auto& activeDocument = CurrentDocument(state);
            SyncEditorText(activeDocument, ImGui::GetTime());
            if (activeDocument.dirty && (ImGui::GetTime() - activeDocument.lastEditTime) > 0.30)
                RefreshLivePreview(state, activeDocument);
            if (activeDocument.previewText.empty() && state.hasBuildPreview)
                ApplyBuildPreview(state, activeDocument);
            SyncPreviewHighlight(activeDocument, state.preferences);
            ImGui::Separator();
            if (ImFont* font = ChooseMonoFont(state.preferences.editorZoom))
                ImGui::PushFont(font);
            if (state.requestFocusCurrentEditor)
                ImGui::SetKeyboardFocusHere();
            activeDocument.editor->Render("SourceEditor", ImVec2(-1.0f, -1.0f));
            state.requestFocusCurrentEditor = false;
            sourceHovered =
                ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup)
                || ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);
            if (sourceHovered || activeDocument.editor->IsFocused())
                state.activeZoomTarget = ZoomTarget::Source;
            if (ImFont* font = ChooseMonoFont(state.preferences.editorZoom))
                ImGui::PopFont();
        }
        ImGui::EndChild();

        if (sourceHovered && ImGui::GetIO().KeyCtrl && ImGui::GetIO().MouseWheel != 0.0f)
        {
            state.preferences.editorZoom = std::clamp(state.preferences.editorZoom + ImGui::GetIO().MouseWheel * 0.1f, 0.75f, 2.5f);
            state.activeZoomTarget = ZoomTarget::Source;
        }

        ImGui::SameLine();
        DrawVerticalSplitter("##PreviewSplitter", &editorWidth, minEditorWidth, minPreviewWidth, totalWidth, availableHeight);
        state.previewWidth = std::max(minPreviewWidth, totalWidth - editorWidth - (state.showOutline ? state.outlineWidth + splitterWidth * 2.0f : splitterWidth));
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_ChildBg, state.preferences.autoItPlus.background);
        ImGui::BeginChild("PreviewPane", ImVec2(0.0f, 0.0f), true);
        bool previewHovered = false;
        {
            ScopedFont previewPaneFont(ChooseMonoFont(1.0f));
            auto& document = CurrentDocument(state);
            DrawSectionHeader("preview", "Generated AU3", &document.previewStatus, icons.iconSecondary);
            ImGui::Separator();
            if (ImFont* font = ChooseMonoFont(state.preferences.previewZoom))
                ImGui::PushFont(font);
            if (document.previewEditor != nullptr)
            {
                document.previewEditor->SetImGuiChildIgnored(true);
                document.previewEditor->Render("PreviewEditor", ImVec2(-1.0f, -1.0f));
            }
            previewHovered =
                ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup)
                || ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);
            if (previewHovered || (document.previewEditor != nullptr && document.previewEditor->IsFocused()))
                state.activeZoomTarget = ZoomTarget::Preview;
            if (ImFont* font = ChooseMonoFont(state.preferences.previewZoom))
                ImGui::PopFont();
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();

        if (previewHovered && ImGui::GetIO().KeyCtrl && ImGui::GetIO().MouseWheel != 0.0f)
        {
            state.preferences.previewZoom = std::clamp(state.preferences.previewZoom + ImGui::GetIO().MouseWheel * 0.1f, 0.75f, 2.5f);
            state.activeZoomTarget = ZoomTarget::Preview;
        }

        ImGui::EndChild();
    }

    void DrawBottomPanel(EditorState& state, const ImVec2& size)
    {
        if (!state.showOutput && !state.showRun)
            return;

        const auto& theme = ActiveTheme(state.preferences);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, theme.uiTheme.panelColor);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 6.0f));
        ImGui::BeginChild("BottomPanel", size, false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        {
            ScopedFont bottomFont(ChooseMonoFont(1.0f));
            if (ImGui::BeginTabBar("BottomTabs"))
            {
                const ImGuiTabItemFlags outputFlags =
                    state.requestedBottomTab == BottomPanelTab::Output ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
                if (state.showOutput && ImGui::BeginTabItem("Output", nullptr, outputFlags))
                {
                    state.activeBottomTab = BottomPanelTab::Output;
                    if (state.outputEditor != nullptr)
                    {
                        if (ImFont* font = ChooseMonoFont(state.preferences.outputZoom))
                            ImGui::PushFont(font);
                        state.outputEditor->Render("OutputEditor", ImVec2(-1.0f, -1.0f));
                        const bool outputHovered =
                            ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup)
                            || ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);
                        if (outputHovered && ImGui::GetIO().KeyCtrl && ImGui::GetIO().MouseWheel != 0.0f)
                        {
                            state.preferences.outputZoom = std::clamp(state.preferences.outputZoom + ImGui::GetIO().MouseWheel * 0.1f, 0.75f, 2.5f);
                            state.activeZoomTarget = ZoomTarget::Output;
                        }
                        if (outputHovered || state.outputEditor->IsFocused())
                            state.activeZoomTarget = ZoomTarget::Output;
                        if (ImFont* font = ChooseMonoFont(state.preferences.outputZoom))
                            ImGui::PopFont();
                    }
                    ImGui::EndTabItem();
                }

                const ImGuiTabItemFlags runFlags =
                    state.requestedBottomTab == BottomPanelTab::Run ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
                if (state.showRun && ImGui::BeginTabItem("Run", nullptr, runFlags))
                {
                    state.activeBottomTab = BottomPanelTab::Run;
                    if (state.runEditor != nullptr)
                    {
                        if (ImFont* font = ChooseMonoFont(state.preferences.outputZoom))
                            ImGui::PushFont(font);
                        state.runEditor->Render("RunOutputEditor", ImVec2(-1.0f, -1.0f));
                        const bool runHovered =
                            ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup)
                            || ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);
                        if (runHovered && ImGui::GetIO().KeyCtrl && ImGui::GetIO().MouseWheel != 0.0f)
                        {
                            state.preferences.outputZoom = std::clamp(state.preferences.outputZoom + ImGui::GetIO().MouseWheel * 0.1f, 0.75f, 2.5f);
                            state.activeZoomTarget = ZoomTarget::Output;
                        }
                        if (runHovered || state.runEditor->IsFocused())
                            state.activeZoomTarget = ZoomTarget::Output;
                        if (ImFont* font = ChooseMonoFont(state.preferences.outputZoom))
                            ImGui::PopFont();
                    }
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
        }
        state.requestedBottomTab.reset();
        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();
    }

    void DrawStatusBar(EditorState& state)
    {
        const auto& theme = ActiveTheme(state.preferences);
        if (!HasOpenDocument(state))
        {
            ImGui::PushStyleColor(ImGuiCol_ChildBg, theme.uiTheme.panelAltColor);
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 3.0f));
            ImGui::BeginChild("StatusBar", ImVec2(0.0f, 22.0f), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            {
                ScopedFont statusFont(ChooseUiFont(0.95f));
                if (state.project.has_value())
                {
                    ImGui::Text("%s", state.project->name.c_str());
                    DrawInlineDivider();
                    ImGui::Text("%s", BuildConfigurationLabel(state.buildConfiguration));
                }
                else
                {
                    ImGui::TextUnformatted("No project loaded");
                }
            }
            ImGui::EndChild();
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor();
            return;
        }

        const auto& document = CurrentDocument(state);
        const auto cursor = document.editor->GetCursorPosition();
        ImGui::PushStyleColor(ImGuiCol_ChildBg, theme.uiTheme.panelAltColor);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 3.0f));
        ImGui::BeginChild("StatusBar", ImVec2(0.0f, 22.0f), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        {
            ScopedFont statusFont(ChooseUiFont(0.95f));
            ImGui::Text("Line %d, Col %d", cursor.mLine + 1, cursor.mColumn + 1);
            DrawInlineDivider();
            ImGui::Text("%d lines", document.editor->GetTotalLines());
            DrawInlineDivider();
            ImGui::Text("%s", document.dirty ? "Modified" : "Saved");
            DrawInlineDivider();
            ImGui::Text("%s", OutputKindLabel(document.outputKind));
            if (state.project.has_value())
            {
                DrawInlineDivider();
                ImGui::Text("%s", BuildConfigurationLabel(state.buildConfiguration));
            }
            DrawInlineDivider();
            ImGui::Text("%s", document.path.extension().string().c_str());
        }
        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();
    }

    void SetupStyle(const EditorPreferences& preferences)
    {
        const auto& theme = ActiveTheme(preferences);
        ImGui::StyleColorsDark();
        auto& style = ImGui::GetStyle();
        style.WindowRounding = 10.0f;
        style.ChildRounding = 10.0f;
        style.FrameRounding = 7.0f;
        style.GrabRounding = 6.0f;
        style.TabRounding = 7.0f;
        style.WindowBorderSize = 1.0f;
        style.FrameBorderSize = 1.0f;
        style.PopupRounding = 8.0f;
        style.ScrollbarRounding = 7.0f;
        style.WindowPadding = ImVec2(6.0f, 6.0f);
        style.ItemSpacing = ImVec2(6.0f, 6.0f);
        style.ItemInnerSpacing = ImVec2(4.0f, 3.0f);
        style.Colors[ImGuiCol_Text] = theme.uiTheme.textColor;
        style.Colors[ImGuiCol_TextDisabled] = theme.uiTheme.textDisabledColor;
        style.Colors[ImGuiCol_WindowBg] = theme.uiTheme.windowBg;
        style.Colors[ImGuiCol_PopupBg] = theme.uiTheme.popupBg;
        style.Colors[ImGuiCol_MenuBarBg] = theme.uiTheme.menuBarBg;
        style.Colors[ImGuiCol_Border] = theme.uiTheme.borderColor;
        style.Colors[ImGuiCol_Header] = theme.uiTheme.accentSoftColor;
        style.Colors[ImGuiCol_HeaderHovered] = theme.uiTheme.headerHovered;
        style.Colors[ImGuiCol_Button] = theme.uiTheme.accentSoftColor;
        style.Colors[ImGuiCol_ButtonHovered] = theme.uiTheme.buttonHovered;
        style.Colors[ImGuiCol_ButtonActive] = theme.uiTheme.buttonActive;
        style.Colors[ImGuiCol_FrameBg] = theme.uiTheme.frameBg;
        style.Colors[ImGuiCol_FrameBgHovered] = theme.uiTheme.frameBgHovered;
        style.Colors[ImGuiCol_Tab] = theme.uiTheme.tabColor;
        style.Colors[ImGuiCol_TabHovered] = theme.uiTheme.tabHovered;
        style.Colors[ImGuiCol_TabSelected] = theme.uiTheme.accentSoftColor;
        style.Colors[ImGuiCol_TitleBg] = theme.uiTheme.titleBg;
        style.Colors[ImGuiCol_TitleBgActive] = theme.uiTheme.titleBgActive;
        style.Colors[ImGuiCol_Separator] = theme.uiTheme.separatorColor;
        style.Colors[ImGuiCol_ResizeGrip] = theme.uiTheme.resizeGrip;
        style.Colors[ImGuiCol_ResizeGripHovered] = theme.uiTheme.accentColor;
        style.Colors[ImGuiCol_CheckMark] = theme.uiTheme.accentColor;
        style.Colors[ImGuiCol_SliderGrab] = theme.uiTheme.accentColor;
        style.Colors[ImGuiCol_SliderGrabActive] = theme.uiTheme.sliderGrabActive;
    }
}

namespace AutoItPlus::Editor
{
    int RunEditorApp()
    {
        if (glfwInit() == GLFW_FALSE)
            return 1;

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        GLFWwindow* window = glfwCreateWindow(1600, 900, "Torii Labs", nullptr, nullptr);
        if (window == nullptr)
        {
            glfwTerminate();
            return 1;
        }

        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.IniFilename = nullptr;
        EditorState state;
        LoadEditorPreferences(state.preferences);
        LoadShortcutOverrides(state.shortcutOverrides);
        SetupStyle(state.preferences);
        LoadEditorFonts();

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 330");

        state.outputEditor = CreateConsoleWidget(state.preferences);
        SetLoggerText(*state.outputEditor, state.outputLog);
        state.runEditor = CreateConsoleWidget(state.preferences);
        SetLoggerText(*state.runEditor, state.runOutput);
        ApplyPreferences(state);
        ConfigureDefaultHotkeys(state);
        SaveShortcutOverrides(state.shortcutOverrides);

        while (!glfwWindowShouldClose(window) && !state.requestExit)
        {
            glfwPollEvents();

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            try
            {
                const ImGuiViewport* viewport = ImGui::GetMainViewport();
                ImGui::SetNextWindowPos(viewport->WorkPos);
                ImGui::SetNextWindowSize(viewport->WorkSize);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 4.0f));
                ImGui::Begin("EditorShell", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                ImGui::PopStyleVar(3);

                if (ImGui::BeginMenuBar())
                {
                    DrawMenuContents(state);
                    ImGui::EndMenuBar();
                }

                HandleShortcuts(state);
                PollBuildTask(state);
                PollRunTask(state);
                DrawUnsavedDialog(state);
                DrawPreferences(state);
                DrawProjectSettings(state);

                const float statusBarHeight = 22.0f;
                ImGui::BeginChild("MainContent", ImVec2(0.0f, -statusBarHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                if (state.showInspector)
                {
                    const float totalWidth = ImGui::GetContentRegionAvail().x;
                    DrawSidebar(state, ImVec2(state.sidebarWidth, 0.0f));
                    ImGui::SameLine();
                    DrawVerticalSplitter("##SidebarSplitter", &state.sidebarWidth, 220.0f, 680.0f, totalWidth, ImGui::GetContentRegionAvail().y);
                    ImGui::SameLine();
                }

                ImGui::BeginChild("CenterColumn", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                const float availableHeight = ImGui::GetContentRegionAvail().y;
                if (state.showOutput || state.showTokens || state.showRun)
                {
                    constexpr float kBottomSplitterHeight = 12.0f;
                    float workspaceHeight = std::max(240.0f, availableHeight - state.bottomPanelHeight - kBottomSplitterHeight);
                    DrawWorkspace(state, ImVec2(0.0f, workspaceHeight));
                    DrawHorizontalSplitter("##BottomSplitter", &workspaceHeight, 240.0f, 96.0f, availableHeight);
                    state.bottomPanelHeight = std::max(96.0f, availableHeight - workspaceHeight - kBottomSplitterHeight);
                    DrawBottomPanel(state, ImVec2(0.0f, state.bottomPanelHeight));
                }
                else
                {
                    DrawWorkspace(state, ImVec2(0.0f, 0.0f));
                }
                ImGui::EndChild();
                ImGui::EndChild();

                DrawFileActionDialog(state);
                if (state.requestedOpenPath.has_value())
                {
                    const auto path = *state.requestedOpenPath;
                    state.requestedOpenPath.reset();
                    OpenDocumentInEditor(state, path);
                }
                DrawStatusBar(state);
                ImGui::End();
            }
            catch (const std::exception& exception)
            {
                state.outputLog += "[ERROR] " + std::string(exception.what()) + "\n";
                if (state.outputEditor != nullptr)
                    SetLoggerText(*state.outputEditor, state.outputLog);
            }

            ImGui::Render();
            int displayWidth = 0;
            int displayHeight = 0;
            glfwGetFramebufferSize(window, &displayWidth, &displayHeight);
            glViewport(0, 0, displayWidth, displayHeight);
            glClearColor(0.05f, 0.06f, 0.08f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            glfwSwapBuffers(window);
        }

        SaveProjectWorkspace(state);
        SaveEditorPreferences(state.preferences);
        SaveShortcutOverrides(state.shortcutOverrides);
        DestroyIconTextures();

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 0;
    }
}

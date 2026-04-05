#pragma once

#include "imgui.h"

#include <cstddef>
#include <functional>

namespace AutoItPlus::Editor::Widgets
{
    struct SocialLinkButton
    {
        const char* id = "";
        const char* iconName = "";
        const char* tooltip = "";
        const char* url = "";
        ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    };

    using IconDrawCallback = std::function<void(const char*, float, const ImVec4&)>;

    bool OpenExternalLink(const char* url);
    bool DrawSocialIconButton(const SocialLinkButton& button, const IconDrawCallback& drawIcon, float buttonSize = 28.0f, float iconSize = 14.0f);
    void DrawSocialIconRow(const SocialLinkButton* buttons, std::size_t count, const IconDrawCallback& drawIcon, float buttonSize = 28.0f, float iconSize = 14.0f);
} // namespace AutoItPlus::Editor::Widgets

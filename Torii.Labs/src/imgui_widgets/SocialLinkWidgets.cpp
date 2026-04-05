#include "SocialLinkWidgets.h"

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#endif

#include <cstdint>
#include <string>

namespace AutoItPlus::Editor::Widgets
{
    bool OpenExternalLink(const char* url)
    {
        if (url == nullptr || url[0] == '\0')
            return false;

#if defined(_WIN32)
        const auto result = reinterpret_cast<std::intptr_t>(ShellExecuteA(nullptr, "open", url, nullptr, nullptr, SW_SHOWNORMAL));
        return result > 32;
#else
        (void)url;
        return false;
#endif
    }

    bool DrawSocialIconButton(const SocialLinkButton& button, const IconDrawCallback& drawIcon, float buttonSize, float iconSize)
    {
        const ImVec2 size(buttonSize, buttonSize);
        const std::string widgetId = std::string("##") + button.id;
        const bool pressed = ImGui::Button(widgetId.c_str(), size);
        const ImVec2 rectMin = ImGui::GetItemRectMin();
        const float iconOffset = (buttonSize - iconSize) * 0.5f;

        ImGui::SetCursorScreenPos(ImVec2(rectMin.x + iconOffset, rectMin.y + iconOffset));
        drawIcon(button.iconName, iconSize, button.color);
        ImGui::SetCursorScreenPos(ImVec2(rectMin.x, rectMin.y + buttonSize));

        if (ImGui::IsItemHovered() && button.tooltip[0] != '\0')
            ImGui::SetTooltip("%s", button.tooltip);

        if (pressed)
            return OpenExternalLink(button.url);
        return false;
    }

    void DrawSocialIconRow(const SocialLinkButton* buttons, std::size_t count, const IconDrawCallback& drawIcon, float buttonSize, float iconSize)
    {
        if (buttons == nullptr || count == 0)
            return;

        const float rowStartY = ImGui::GetCursorPosY();
        for (std::size_t index = 0; index < count; ++index)
        {
            if (index > 0)
            {
                ImGui::SameLine(0.0f, 6.0f);
                ImGui::SetCursorPosY(rowStartY);
            }
            DrawSocialIconButton(buttons[index], drawIcon, buttonSize, iconSize);
        }
    }
} // namespace AutoItPlus::Editor::Widgets

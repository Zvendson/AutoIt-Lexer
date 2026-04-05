#include "ConsoleWidget.h"

#include "imgui.h"

#include <utility>

ConsoleWidget::ConsoleWidget(std::unique_ptr<TextEditor> editor)
	: mEditor(std::move(editor))
{
}

void ConsoleWidget::Render(const char* title, const ImVec2& size, bool border)
{
	const auto background = ImGui::ColorConvertU32ToFloat4(
		mEditor->GetPalette()[static_cast<std::size_t>(TextEditor::PaletteIndex::Background)]);
	ImGui::PushStyleColor(ImGuiCol_ChildBg, background);
	ImGui::BeginChild(title, size, border, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_NoMove);
	mEditor->SetImGuiChildIgnored(true);
	mEditor->Render("##ConsoleWidget", ImVec2(-1.0f, -1.0f));
	mFollowOutput = IsScrolledToBottom();
	ImGui::EndChild();
	ImGui::PopStyleColor();
}

void ConsoleWidget::SetText(const std::string& text)
{
	ReplaceText(text, false);
}

void ConsoleWidget::SetAnsiText(const std::string& text)
{
	ReplaceText(text, true);
}

void ConsoleWidget::SetLoggerText(const std::string& text)
{
	if (text.find('\x1b') != std::string::npos)
		SetAnsiText(text);
	else
		SetText(text);
}

void ConsoleWidget::SetShowWhitespaces(bool value)
{
	mEditor->SetShowWhitespaces(value);
}

void ConsoleWidget::SetShowLineNumbers(bool value)
{
	mEditor->SetShowLineNumbers(value);
}

void ConsoleWidget::SetReadOnly(bool value)
{
	mEditor->SetReadOnly(value);
}

bool ConsoleWidget::IsFocused() const
{
	return mEditor->IsFocused();
}

TextEditor& ConsoleWidget::GetEditor()
{
	return *mEditor;
}

const TextEditor& ConsoleWidget::GetEditor() const
{
	return *mEditor;
}

void ConsoleWidget::ReplaceText(const std::string& text, bool ansi)
{
	if (ansi)
		mEditor->SetAnsiText(text, false);
	else
		mEditor->SetText(text, false);

	if (mFollowOutput)
		mEditor->RequestScrollToBottom();
}

bool ConsoleWidget::IsScrolledToBottom() const
{
	return ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0f;
}

#pragma once

#include "TextEditor.h"

#include <memory>
#include <string>

class ConsoleWidget
{
public:
	explicit ConsoleWidget(std::unique_ptr<TextEditor> editor);

	void Render(const char* title, const ImVec2& size = ImVec2(), bool border = false);

	void SetText(const std::string& text);
	void SetAnsiText(const std::string& text);
	void SetLoggerText(const std::string& text);

	void SetShowWhitespaces(bool value);
	void SetShowLineNumbers(bool value);
	void SetReadOnly(bool value);
	bool IsFocused() const;

	TextEditor& GetEditor();
	const TextEditor& GetEditor() const;

private:
	void ReplaceText(const std::string& text, bool ansi);
	bool IsScrolledToBottom() const;

	std::unique_ptr<TextEditor> mEditor;
	bool mFollowOutput = true;
};

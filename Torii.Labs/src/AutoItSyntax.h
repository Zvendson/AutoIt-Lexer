#pragma once

#include "EditorState.h"

#include "ConsoleWidget.h"
#include "TextEditor.h"

#include <memory>

namespace AutoItPlus::Editor
{
    TextEditor::LanguageDefinition BuildLanguageDefinition(SyntaxFlavor flavor);
    TextEditor::Palette BuildPalette(SyntaxFlavor flavor, const EditorPreferences& preferences);
    void ApplySyntaxFlavor(TextEditor& editor, SyntaxFlavor flavor, const EditorPreferences& preferences);
    void ApplySyntaxFlavor(ConsoleWidget& editor, SyntaxFlavor flavor, const EditorPreferences& preferences);
    SyntaxFlavor DetermineSourceSyntax(const std::filesystem::path& path);
    void SetLoggerText(TextEditor& editor, const std::string& text);
    void SetLoggerText(ConsoleWidget& editor, const std::string& text);
    std::unique_ptr<TextEditor> CreateTextEditor(SyntaxFlavor flavor, const EditorPreferences& preferences, bool readOnly = false);
    std::unique_ptr<ConsoleWidget> CreateConsoleWidget(const EditorPreferences& preferences);
}

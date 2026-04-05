#include "AutoItSyntax.h"

#include "AutoItPreprocessor/Tokenizer/Token.h"
#include "AutoItPreprocessor/Tokenizer/Tokenizer.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <string>

namespace
{
    float Luminance(const ImVec4& color)
    {
        return color.x * 0.2126f + color.y * 0.7152f + color.z * 0.0722f;
    }

    ImVec4 AdjustColor(const ImVec4& color, float delta)
    {
        return ImVec4(
            std::clamp(color.x + delta, 0.0f, 1.0f),
            std::clamp(color.y + delta, 0.0f, 1.0f),
            std::clamp(color.z + delta, 0.0f, 1.0f),
            color.w);
    }

    ImU32 ToColor(const ImVec4& color)
    {
        return ImGui::ColorConvertFloat4ToU32(color);
    }

    TextEditor::PaletteIndex ToPaletteIndex(AutoItPreprocessor::Tokenizer::TokenKind kind)
    {
        using AutoItPreprocessor::Tokenizer::TokenKind;
        using PaletteIndex = TextEditor::PaletteIndex;

        switch (kind)
        {
            case TokenKind::Keyword:
                return PaletteIndex::Keyword;
            case TokenKind::Decimals:
            case TokenKind::Hex:
                return PaletteIndex::Number;
            case TokenKind::String:
                return PaletteIndex::String;
            case TokenKind::Comment:
            case TokenKind::MultiComment:
                return PaletteIndex::Comment;
            case TokenKind::AutoItCommand:
                return PaletteIndex::Preprocessor;
            case TokenKind::Variable:
            case TokenKind::Macro:
                return PaletteIndex::KnownIdentifier;
            case TokenKind::Word:
                return PaletteIndex::Identifier;
            case TokenKind::Object:
            case TokenKind::OpenedParen:
            case TokenKind::ClosedParen:
            case TokenKind::OpenedSquare:
            case TokenKind::ClosedSquare:
            case TokenKind::LessThan:
            case TokenKind::GreaterThan:
            case TokenKind::Equal:
            case TokenKind::Plus:
            case TokenKind::Minus:
            case TokenKind::Asterisk:
            case TokenKind::Slash:
            case TokenKind::Power:
            case TokenKind::Comma:
            case TokenKind::Colon:
            case TokenKind::Concatenate:
            case TokenKind::Questionmark:
                return PaletteIndex::Punctuation;
            case TokenKind::Space:
            case TokenKind::Tab:
            case TokenKind::LineFeed:
            case TokenKind::Multiline:
            case TokenKind::Custom:
            case TokenKind::Start:
            case TokenKind::End:
            case TokenKind::Error:
                return PaletteIndex::Default;
        }

        return PaletteIndex::Default;
    }

    bool TokenizeAutoIt(
        const char* inBegin,
        const char* inEnd,
        const char*& outBegin,
        const char*& outEnd,
        TextEditor::PaletteIndex& paletteIndex)
    {
        if (inBegin >= inEnd)
            return false;

        AutoItPreprocessor::Tokenizer::Tokenizer tokenizer(std::string(inBegin, inEnd));
        const auto token = tokenizer.Next();
        if (token.Is(AutoItPreprocessor::Tokenizer::TokenKind::End))
            return false;

        const auto startOffset = token.GetStart();
        std::size_t endOffset = token.GetEnd();
        if (endOffset <= startOffset)
            endOffset = startOffset + 1U;

        outBegin = inBegin + static_cast<std::ptrdiff_t>(startOffset);
        outEnd = inBegin + static_cast<std::ptrdiff_t>(std::min(endOffset, static_cast<std::size_t>(inEnd - inBegin)));
        paletteIndex = ToPaletteIndex(token.GetKind());
        return outBegin < outEnd;
    }

    bool TokenizeLogger(
        const char* inBegin,
        const char* inEnd,
        const char*& outBegin,
        const char*& outEnd,
        TextEditor::PaletteIndex& paletteIndex)
    {
        if (inBegin >= inEnd)
            return false;

        outBegin = inBegin;
        outEnd = inEnd;
        paletteIndex = TextEditor::PaletteIndex::Default;

        const std::string line(inBegin, inEnd);
        const auto containsAnsi = line.find('\x1b') != std::string::npos;
        if (line.rfind("> ", 0) == 0)
            paletteIndex = TextEditor::PaletteIndex::KnownIdentifier;
        else if (line.rfind("[STDERR] ", 0) == 0)
            paletteIndex = TextEditor::PaletteIndex::String;
        else if (line.rfind("Finished with exit code ", 0) == 0)
        {
            const auto lastSpace = line.find_last_of(' ');
            const bool success = lastSpace != std::string::npos && line.substr(lastSpace + 1) == "0";
            paletteIndex = success ? TextEditor::PaletteIndex::Identifier : TextEditor::PaletteIndex::String;
        }
        if (line.find("[ERROR]") != std::string::npos || line.find("Failed") != std::string::npos)
            paletteIndex = TextEditor::PaletteIndex::String;
        else if (line.find("[WARN]") != std::string::npos || line.find("[WARNING]") != std::string::npos)
            paletteIndex = TextEditor::PaletteIndex::Number;
        else if (line.find("[DEBUG]") != std::string::npos)
            paletteIndex = TextEditor::PaletteIndex::KnownIdentifier;
        else if (line.find("[INFO]") != std::string::npos || line.find("Built ") != std::string::npos || line.find("Loaded ") != std::string::npos)
            paletteIndex = TextEditor::PaletteIndex::Identifier;
        else if (containsAnsi)
        {
            if (line.find("[31m") != std::string::npos || line.find("[91m") != std::string::npos)
                paletteIndex = TextEditor::PaletteIndex::String;
            else if (line.find("[33m") != std::string::npos || line.find("[93m") != std::string::npos)
                paletteIndex = TextEditor::PaletteIndex::Number;
            else if (line.find("[34m") != std::string::npos || line.find("[94m") != std::string::npos)
                paletteIndex = TextEditor::PaletteIndex::KnownIdentifier;
            else
                paletteIndex = TextEditor::PaletteIndex::Identifier;
        }

        return true;
    }

    TextEditor::LanguageDefinition MakeAutoItLanguageDefinition(const char* name)
    {
        TextEditor::LanguageDefinition language;
        language.mName = name;
        language.mCaseSensitive = false;
        language.mAutoIndentation = true;
        language.mPreprocChar = '#';
        language.mCommentStart = "#cs";
        language.mCommentEnd = "#ce";
        language.mSingleLineComment = ";";
        language.mTokenize = &TokenizeAutoIt;

        static constexpr const char* keywords[] = {
            "and", "byref", "case", "const", "continuecase", "continueloop", "default",
            "dim", "do", "else", "elseif", "endfunc", "endif", "endselect", "endswitch",
            "endwith", "enum", "exit", "exitloop", "false", "for", "func", "global", "if",
            "in", "local", "next", "not", "null", "or", "redim", "return", "select", "step",
            "switch", "then", "to", "true", "until", "volatile", "wend", "while", "with"
        };

        for (const char* keyword : keywords)
            language.mKeywords.insert(keyword);

        static constexpr const char* builtins[] = {
            "ConsoleWrite", "ConsoleWriteError", "FileExists", "FileRead", "FileWrite",
            "IniRead", "IniWrite", "MsgBox", "Run", "RunWait", "ShellExecute",
            "StringFormat", "StringInStr", "StringLeft", "StringLen", "StringMid",
            "StringReplace", "StringSplit", "UBound"
        };

        for (const char* builtin : builtins)
        {
            TextEditor::Identifier identifier;
            identifier.mDeclaration = "Built-in function";
            language.mIdentifiers.emplace(builtin, identifier);
        }

        static constexpr const char* directives[] = {
            "include", "include-once", "pragma", "region", "endregion", "comments-start",
            "comments-end", "cs", "ce"
        };

        for (const char* directive : directives)
        {
            TextEditor::Identifier identifier;
            identifier.mDeclaration = "AutoIt directive";
            language.mPreprocIdentifiers.emplace(directive, identifier);
        }

        return language;
    }

    TextEditor::LanguageDefinition MakeLoggerLanguageDefinition()
    {
        TextEditor::LanguageDefinition language;
        language.mName = "Logger";
        language.mCaseSensitive = false;
        language.mAutoIndentation = false;
        language.mTokenize = &TokenizeLogger;
        return language;
    }

    TextEditor::Palette MakePaletteFromSettings(const AutoItPlus::Editor::SyntaxPaletteSettings& settings)
    {
        auto palette = TextEditor::GetDarkPalette();
        palette[static_cast<std::size_t>(TextEditor::PaletteIndex::Background)] = ToColor(settings.background);
        palette[static_cast<std::size_t>(TextEditor::PaletteIndex::CurrentLineFill)] = ToColor(settings.currentLine);
        palette[static_cast<std::size_t>(TextEditor::PaletteIndex::CurrentLineFillInactive)] = ToColor(settings.currentLine);
        palette[static_cast<std::size_t>(TextEditor::PaletteIndex::LineNumber)] = ToColor(settings.lineNumber);
        palette[static_cast<std::size_t>(TextEditor::PaletteIndex::Cursor)] = ToColor(ImVec4(0.95f, 0.97f, 1.0f, 1.0f));
        palette[static_cast<std::size_t>(TextEditor::PaletteIndex::Selection)] = ToColor(settings.selection);
        palette[static_cast<std::size_t>(TextEditor::PaletteIndex::CurrentLineEdge)] = ToColor(settings.currentLineEdge);
        palette[static_cast<std::size_t>(TextEditor::PaletteIndex::Keyword)] = ToColor(settings.keyword);
        palette[static_cast<std::size_t>(TextEditor::PaletteIndex::KnownIdentifier)] = ToColor(settings.knownIdentifier);
        palette[static_cast<std::size_t>(TextEditor::PaletteIndex::Preprocessor)] = ToColor(settings.preprocessor);
        palette[static_cast<std::size_t>(TextEditor::PaletteIndex::PreprocIdentifier)] = ToColor(settings.preprocessor);
        palette[static_cast<std::size_t>(TextEditor::PaletteIndex::Comment)] = ToColor(settings.comment);
        palette[static_cast<std::size_t>(TextEditor::PaletteIndex::MultiLineComment)] = ToColor(settings.comment);
        palette[static_cast<std::size_t>(TextEditor::PaletteIndex::String)] = ToColor(settings.string);
        palette[static_cast<std::size_t>(TextEditor::PaletteIndex::Number)] = ToColor(settings.number);
        palette[static_cast<std::size_t>(TextEditor::PaletteIndex::Punctuation)] = ToColor(settings.punctuation);
        palette[static_cast<std::size_t>(TextEditor::PaletteIndex::Identifier)] = ToColor(settings.identifier);
        return palette;
    }

    TextEditor::Palette MakeLoggerPalette(const AutoItPlus::Editor::EditorPreferences& preferences)
    {
        auto palette = MakePaletteFromSettings(preferences.autoIt);
        const bool lightTheme = Luminance(preferences.uiTheme.panelColor) > 0.6f;
        const ImVec4 defaultText = lightTheme ? ImVec4(0.12f, 0.16f, 0.20f, 1.0f) : ImVec4(0.88f, 0.92f, 0.97f, 1.0f);
        // Keep logger/output editors visually aligned with the main code editor surface.
        const ImVec4 background = preferences.autoItPlus.background;
        const ImVec4 currentLine = lightTheme ? AdjustColor(background, -0.06f) : AdjustColor(background, 0.04f);
        palette[static_cast<std::size_t>(TextEditor::PaletteIndex::Default)] = ToColor(defaultText);
        palette[static_cast<std::size_t>(TextEditor::PaletteIndex::Identifier)] = ToColor(defaultText);
        palette[static_cast<std::size_t>(TextEditor::PaletteIndex::KnownIdentifier)] = ToColor(preferences.autoIt.preprocessor);
        palette[static_cast<std::size_t>(TextEditor::PaletteIndex::Number)] = ToColor(preferences.autoIt.number);
        palette[static_cast<std::size_t>(TextEditor::PaletteIndex::String)] = ToColor(preferences.autoIt.string);
        palette[static_cast<std::size_t>(TextEditor::PaletteIndex::Comment)] = ToColor(preferences.autoIt.comment);
        palette[static_cast<std::size_t>(TextEditor::PaletteIndex::Background)] = ToColor(background);
        palette[static_cast<std::size_t>(TextEditor::PaletteIndex::CurrentLineFill)] = ToColor(ImVec4(currentLine.x, currentLine.y, currentLine.z, 0.55f));
        palette[static_cast<std::size_t>(TextEditor::PaletteIndex::CurrentLineFillInactive)] = ToColor(ImVec4(currentLine.x, currentLine.y, currentLine.z, 0.35f));
        palette[static_cast<std::size_t>(TextEditor::PaletteIndex::LineNumber)] = ToColor(preferences.autoIt.lineNumber);
        palette[static_cast<std::size_t>(TextEditor::PaletteIndex::Cursor)] = ToColor(lightTheme ? ImVec4(0.18f, 0.26f, 0.34f, 1.0f) : ImVec4(0.95f, 0.97f, 1.0f, 1.0f));
        palette[static_cast<std::size_t>(TextEditor::PaletteIndex::Selection)] = ToColor(preferences.autoIt.selection);
        palette[static_cast<std::size_t>(TextEditor::PaletteIndex::CurrentLineEdge)] = ToColor(preferences.autoIt.currentLineEdge);
        return palette;
    }
}

namespace AutoItPlus::Editor
{
    void SetLoggerText(TextEditor& editor, const std::string& text)
    {
        if (text.find('\x1b') != std::string::npos)
            editor.SetAnsiText(text);
        else
            editor.SetText(text);
    }

    void SetLoggerText(ConsoleWidget& editor, const std::string& text)
    {
        editor.SetLoggerText(text);
    }

    TextEditor::LanguageDefinition BuildLanguageDefinition(SyntaxFlavor flavor)
    {
        switch (flavor)
        {
            case SyntaxFlavor::AutoIt:
                return MakeAutoItLanguageDefinition("AutoIt");
            case SyntaxFlavor::AutoItPlus:
                return MakeAutoItLanguageDefinition("AutoIt+");
            case SyntaxFlavor::Logger:
                return MakeLoggerLanguageDefinition();
        }

        return MakeAutoItLanguageDefinition("AutoIt+");
    }

    TextEditor::Palette BuildPalette(SyntaxFlavor flavor, const EditorPreferences& preferences)
    {
        switch (flavor)
        {
            case SyntaxFlavor::AutoIt:
                return MakePaletteFromSettings(preferences.autoIt);
            case SyntaxFlavor::AutoItPlus:
                return MakePaletteFromSettings(preferences.autoItPlus);
            case SyntaxFlavor::Logger:
                return MakeLoggerPalette(preferences);
        }

        return MakePaletteFromSettings(preferences.autoItPlus);
    }

    void ApplySyntaxFlavor(TextEditor& editor, SyntaxFlavor flavor, const EditorPreferences& preferences)
    {
        editor.SetLanguageDefinition(BuildLanguageDefinition(flavor));
        editor.SetPalette(BuildPalette(flavor, preferences));
    }

    void ApplySyntaxFlavor(ConsoleWidget& editor, SyntaxFlavor flavor, const EditorPreferences& preferences)
    {
        ApplySyntaxFlavor(editor.GetEditor(), flavor, preferences);
    }

    SyntaxFlavor DetermineSourceSyntax(const std::filesystem::path& path)
    {
        return path.extension() == ".au3" ? SyntaxFlavor::AutoIt : SyntaxFlavor::AutoItPlus;
    }

    std::unique_ptr<TextEditor> CreateTextEditor(SyntaxFlavor flavor, const EditorPreferences& preferences, bool readOnly)
    {
        auto editor = std::make_unique<TextEditor>();
        ApplySyntaxFlavor(*editor, flavor, preferences);
        editor->SetTabSize(4);
        editor->SetShowWhitespaces(false);
        editor->SetReadOnly(readOnly);
        return editor;
    }

    std::unique_ptr<ConsoleWidget> CreateConsoleWidget(const EditorPreferences& preferences)
    {
        return std::make_unique<ConsoleWidget>(CreateTextEditor(SyntaxFlavor::Logger, preferences, true));
    }
}

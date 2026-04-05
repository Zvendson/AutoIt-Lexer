#include "EditorServices.h"

#include "AutoItSyntax.h"
#include "SymbolAnalysis.h"
#include "settings/ProjectSettingsFile.h"
#include "settings/ShortcutSettingsFile.h"
#include "settings/UserSettingsFile.h"
#include "settings/WorkspaceSettingsFile.h"

#include "AutoItPreprocessor/Common/SourceDocument.h"
#include "AutoItPreprocessor/Compiler/Compiler.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>
#endif

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <future>
#include <thread>
#include <sstream>
#include <stdexcept>
#include <string>

namespace
{
    std::filesystem::path GetProjectCodeDirectory(const AutoItPlus::Editor::ProjectState& project)
    {
        return project.rootDirectory / "code";
    }

    std::filesystem::path GetProjectMainFilePath(const AutoItPlus::Editor::ProjectState& project)
    {
        if (!project.mainFilePath.empty())
            return project.mainFilePath;
        return GetProjectCodeDirectory(project) / "Main.aup";
    }

    std::filesystem::path GetProjectBuildOutputFilePath(const AutoItPlus::Editor::ProjectState& project, AutoItPlus::Editor::BuildConfiguration configuration)
    {
        return project.rootDirectory
            / "build"
            / (configuration == AutoItPlus::Editor::BuildConfiguration::Release ? "release" : "debug")
            / (project.name + ".au3");
    }

    std::optional<std::filesystem::path> ResolveProjectFileSelection(const std::filesystem::path& selectedPath)
    {
        namespace fs = std::filesystem;

        if (selectedPath.empty())
            return std::nullopt;

        if (fs::is_regular_file(selectedPath))
        {
            if (selectedPath.extension() == AutoItPlus::Editor::kProjectExtension)
                return selectedPath;
            return std::nullopt;
        }

        if (!fs::is_directory(selectedPath))
            return std::nullopt;

        std::vector<fs::path> projectFiles;
        for (const auto& entry : fs::directory_iterator(selectedPath))
        {
            if (entry.is_regular_file() && entry.path().extension() == AutoItPlus::Editor::kProjectExtension)
                projectFiles.push_back(entry.path());
        }

        if (projectFiles.empty())
            return std::nullopt;

        const fs::path preferred = selectedPath / (selectedPath.filename().string() + AutoItPlus::Editor::kProjectExtension);
        if (fs::is_regular_file(preferred))
            return preferred;

        std::sort(projectFiles.begin(), projectFiles.end());
        return projectFiles.front();
    }

    std::string Trim(const std::string& value)
    {
        const auto begin = value.find_first_not_of(" \t\r\n");
        if (begin == std::string::npos)
            return {};

        const auto end = value.find_last_not_of(" \t\r\n");
        return value.substr(begin, end - begin + 1U);
    }

    std::string SanitizeUtf8Lossy(const std::string& text)
    {
        std::string sanitized;
        sanitized.reserve(text.size());

        for (std::size_t index = 0; index < text.size();)
        {
            const unsigned char ch = static_cast<unsigned char>(text[index]);
            std::size_t length = 0;
            if (ch <= 0x7F)
                length = 1;
            else if ((ch & 0xE0) == 0xC0)
                length = 2;
            else if ((ch & 0xF0) == 0xE0)
                length = 3;
            else if ((ch & 0xF8) == 0xF0)
                length = 4;
            else
            {
                ++index;
                continue;
            }

            if (index + length > text.size())
                break;

            bool valid = true;
            for (std::size_t continuation = 1; continuation < length; ++continuation)
            {
                const unsigned char next = static_cast<unsigned char>(text[index + continuation]);
                if ((next & 0xC0) != 0x80)
                {
                    valid = false;
                    break;
                }
            }

            if (!valid)
            {
                ++index;
                continue;
            }

            sanitized.append(text, index, length);
            index += length;
        }

        return sanitized;
    }

    std::string StripUtf8Bom(std::string text)
    {
        if (text.size() >= 3U
            && static_cast<unsigned char>(text[0]) == 0xEF
            && static_cast<unsigned char>(text[1]) == 0xBB
            && static_cast<unsigned char>(text[2]) == 0xBF)
        {
            text.erase(0, 3);
        }

        return text;
    }

    std::string TrimEmptyBoundaryLines(const std::string& text)
    {
        auto isBlankLine = [&](std::size_t lineStart, std::size_t lineEnd) {
            for (std::size_t i = lineStart; i < lineEnd; ++i)
            {
                const char ch = text[i];
                if (ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n')
                    return false;
            }
            return true;
        };

        std::vector<std::pair<std::size_t, std::size_t>> lines;
        std::size_t lineStart = 0;
        while (lineStart < text.size())
        {
            std::size_t lineEnd = lineStart;
            while (lineEnd < text.size() && text[lineEnd] != '\n')
                ++lineEnd;
            const std::size_t sliceEnd = lineEnd < text.size() ? lineEnd + 1U : lineEnd;
            lines.emplace_back(lineStart, sliceEnd);
            lineStart = sliceEnd;
        }

        std::size_t first = 0;
        while (first < lines.size() && isBlankLine(lines[first].first, lines[first].second))
            ++first;

        if (first == lines.size())
            return {};

        std::size_t last = lines.size() - 1U;
        while (last > first && isBlankLine(lines[last].first, lines[last].second))
            --last;

        const std::size_t begin = lines[first].first;
        const std::size_t end = lines[last].second;
        return text.substr(begin, end - begin);
    }

#if defined(_WIN32)
    std::optional<std::wstring> ReadRegistryString(HKEY hive, const wchar_t* subKey, const wchar_t* valueName)
    {
        DWORD valueType = 0;
        DWORD valueSize = 0;
        if (RegGetValueW(hive, subKey, valueName, RRF_RT_REG_SZ, &valueType, nullptr, &valueSize) != ERROR_SUCCESS || valueSize == 0)
            return std::nullopt;

        std::wstring value(valueSize / sizeof(wchar_t), L'\0');
        if (RegGetValueW(hive, subKey, valueName, RRF_RT_REG_SZ, &valueType, value.data(), &valueSize) != ERROR_SUCCESS)
            return std::nullopt;

        value.resize(wcslen(value.c_str()));
        return value;
    }

    std::optional<std::filesystem::path> FindAutoItInterpreterPath()
    {
        constexpr const wchar_t* subKeys[] = {
            L"SOFTWARE\\WOW6432Node\\AutoIt v3\\AutoIt",
            L"SOFTWARE\\AutoIt v3\\AutoIt"
        };

        for (const auto* subKey : subKeys)
        {
            if (const auto installDir = ReadRegistryString(HKEY_LOCAL_MACHINE, subKey, L"InstallDir"))
            {
                const auto installPath = std::filesystem::path(*installDir);
                for (const auto* executableName : { "AutoIt3.exe", "AutoIt3_x64.exe" })
                {
                    const auto candidate = installPath / executableName;
                    if (std::filesystem::is_regular_file(candidate))
                        return candidate;
                }
            }
        }

        return std::nullopt;
    }

    std::string DescribeWindowsError(DWORD errorCode)
    {
        LPSTR messageBuffer = nullptr;
        const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
        const DWORD size = FormatMessageA(flags, nullptr, errorCode, 0, reinterpret_cast<LPSTR>(&messageBuffer), 0, nullptr);
        std::string message = size > 0 && messageBuffer != nullptr ? std::string(messageBuffer, size) : "Unknown Windows error";
        if (messageBuffer != nullptr)
            LocalFree(messageBuffer);
        while (!message.empty() && (message.back() == '\r' || message.back() == '\n'))
            message.pop_back();
        return message;
    }

    void MarkDirectoryHidden(const std::filesystem::path& path)
    {
        const DWORD attributes = GetFileAttributesW(path.wstring().c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES)
            return;
        SetFileAttributesW(path.wstring().c_str(), attributes | FILE_ATTRIBUTE_HIDDEN);
    }
#endif
}

namespace AutoItPlus::Editor
{
    namespace
    {
        std::vector<DocumentState::PreviewLineMapping> BuildPreviewLineMappings(
            const AutoItPreprocessor::Compiler::CompilationUnit& compilation,
            const std::filesystem::path& documentPath)
        {
            std::vector<DocumentState::PreviewLineMapping> mappings;
            if (compilation.lineMappings.empty() && compilation.includeExpansions.empty())
                return mappings;

            const auto normalizedDocumentPath = std::filesystem::absolute(documentPath).lexically_normal();
            for (const auto& mapping : compilation.lineMappings)
            {
                const auto normalizedSourcePath = std::filesystem::absolute(mapping.sourcePath).lexically_normal();
                if (normalizedSourcePath != normalizedDocumentPath || mapping.sourceLine == 0)
                    continue;

                if (mappings.size() <= mapping.sourceLine)
                    mappings.resize(mapping.sourceLine + 1U);

                auto& previewMapping = mappings[mapping.sourceLine];
                if (previewMapping.generatedLineStart == 0 || mapping.generatedLineStart < previewMapping.generatedLineStart)
                    previewMapping.generatedLineStart = mapping.generatedLineStart;
                if (mapping.generatedLineEnd > previewMapping.generatedLineEnd)
                    previewMapping.generatedLineEnd = mapping.generatedLineEnd;
            }

            for (const auto& expansion : compilation.includeExpansions)
            {
                const auto normalizedSourcePath = std::filesystem::absolute(expansion.sourcePath).lexically_normal();
                if (normalizedSourcePath != normalizedDocumentPath || expansion.sourceLine == 0)
                    continue;

                if (mappings.size() <= expansion.sourceLine)
                    mappings.resize(expansion.sourceLine + 1U);

                auto& previewMapping = mappings[expansion.sourceLine];
                previewMapping.isIncludeExpansion = true;
                previewMapping.isSkippedIncludeExpansion = expansion.skipped;
                previewMapping.generatedLineStart = expansion.generatedLineStart;
                previewMapping.generatedLineEnd = expansion.generatedLineEnd;
            }

            return mappings;
        }
    }

    std::string ReadTextFile(const std::filesystem::path& path)
    {
        std::ifstream input(path, std::ios::binary);
        if (!input.is_open())
            throw std::runtime_error("Could not open file: " + path.string());

        return StripUtf8Bom(std::string((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>()));
    }

    void WriteTextFile(const std::filesystem::path& path, const std::string& text)
    {
        if (path.has_parent_path())
            std::filesystem::create_directories(path.parent_path());

        std::ofstream output(path, std::ios::binary);
        if (!output.is_open())
            throw std::runtime_error("Could not write file: " + path.string());

        output << text;
    }

    std::vector<std::filesystem::path> SplitPaths(const std::string& raw)
    {
        std::vector<std::filesystem::path> paths;
        std::stringstream stream(raw);
        std::string part;

        while (std::getline(stream, part, ';'))
        {
            const auto begin = part.find_first_not_of(" \t\r\n");
            if (begin == std::string::npos)
                continue;

            const auto end = part.find_last_not_of(" \t\r\n");
            paths.emplace_back(part.substr(begin, end - begin + 1U));
        }

        return paths;
    }

    std::string JoinPaths(const std::vector<std::filesystem::path>& paths)
    {
        std::string result;
        for (std::size_t index = 0; index < paths.size(); ++index)
        {
            if (index > 0)
                result += ';';
            result += paths[index].generic_string();
        }

        return result;
    }

    std::filesystem::path MakeAbsoluteProjectPath(const ProjectState& project, const std::filesystem::path& path)
    {
        if (path.empty())
            return {};

        if (path.is_absolute())
            return path.lexically_normal();

        return (project.rootDirectory / path).lexically_normal();
    }

    std::filesystem::path MakeProjectRelativePath(const ProjectState& project, const std::filesystem::path& path)
    {
        if (path.empty())
            return {};

        const auto absolute = std::filesystem::absolute(path).lexically_normal();
        const auto root = std::filesystem::absolute(project.rootDirectory).lexically_normal();
        const auto relative = absolute.lexically_relative(root);
        return relative.empty() ? absolute : relative;
    }

    std::string NormalizeProjectPathList(const ProjectState& project, const std::string& raw)
    {
        std::vector<std::filesystem::path> normalized;
        for (const auto& path : SplitPaths(raw))
            normalized.push_back(MakeProjectRelativePath(project, path));
        return JoinPaths(normalized);
    }

    bool IsEditableProjectFile(const std::filesystem::path& path)
    {
        const auto extension = path.extension().string();
        return extension == kSourceExtension || extension == ".au3" || extension == ".tokens" || extension == kProjectExtension || extension == ".txt";
    }

    std::optional<std::filesystem::path> ShowFileDialog(bool saveDialog, const char* filter, const std::filesystem::path& initialPath)
    {
#if defined(_WIN32)
        char fileBuffer[MAX_PATH] = {};
        if (!initialPath.empty())
            strncpy_s(fileBuffer, initialPath.string().c_str(), _TRUNCATE);

        OPENFILENAMEA dialog = {};
        dialog.lStructSize = sizeof(dialog);
        dialog.lpstrFilter = filter;
        dialog.lpstrFile = fileBuffer;
        dialog.nMaxFile = MAX_PATH;
        dialog.lpstrDefExt = "aup";
        dialog.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;

        if (saveDialog)
            dialog.Flags |= OFN_OVERWRITEPROMPT;
        else
            dialog.Flags |= OFN_FILEMUSTEXIST;

        const BOOL result = saveDialog ? GetSaveFileNameA(&dialog) : GetOpenFileNameA(&dialog);
        if (result == FALSE)
            return std::nullopt;

        return std::filesystem::path(fileBuffer);
#else
        (void)saveDialog;
        (void)filter;
        (void)initialPath;
        return std::nullopt;
#endif
    }

    std::filesystem::path MakeDerivedOutputPath(const std::filesystem::path& inputPath, const std::string& suffix)
    {
        const auto extension = inputPath.has_extension() ? inputPath.extension().string() : ".au3";
        return inputPath.parent_path() / (inputPath.stem().string() + suffix + extension);
    }

    std::string BuildTokenView(const std::vector<AutoItPreprocessor::Tokenizer::Token>& tokens)
    {
        std::ostringstream output;
        for (const auto& token : tokens)
        {
            output << token.GetLine() << "  "
                   << AutoItPreprocessor::Tokenizer::ToString(token.GetKind()) << "  "
                   << token.GetContent();

            if (token.Is(AutoItPreprocessor::Tokenizer::TokenKind::Custom))
                output << "  =>  " << token.GetReplacement();

            output << '\n';
        }

        return output.str();
    }

    std::string MakeDocumentTitle(const std::filesystem::path& path)
    {
        return path.filename().empty() ? path.string() : path.filename().string();
    }

    void ApplyDocumentPreferences(DocumentState& document, const EditorPreferences& preferences)
    {
        if (document.editor != nullptr)
        {
            ApplySyntaxFlavor(*document.editor, document.sourceSyntax, preferences);
            document.editor->SetShowWhitespaces(preferences.showWhitespace);
            document.editor->SetShowLineNumbers(preferences.showLineNumbers);
        }

        if (document.previewEditor != nullptr)
        {
            ApplySyntaxFlavor(*document.previewEditor, SyntaxFlavor::AutoIt, preferences);
            document.previewEditor->SetShowWhitespaces(preferences.showWhitespace);
            document.previewEditor->SetShowLineNumbers(preferences.showLineNumbers);
        }
    }

    DocumentState MakeEmptyDocument(const EditorPreferences& preferences)
    {
        DocumentState document;
        document.sourceSyntax = SyntaxFlavor::AutoItPlus;
        document.editor = CreateTextEditor(document.sourceSyntax, preferences);
        document.previewEditor = CreateTextEditor(SyntaxFlavor::AutoIt, preferences, true);
        ApplyDocumentPreferences(document, preferences);
        document.lastSyncedText.clear();
        return document;
    }

    std::string NormalizeTabsToSpaces(const std::string& text, int tabSize)
    {
        std::string normalized;
        normalized.reserve(text.size());

        int column = 0;
        for (const char ch : text)
        {
            if (ch == '\t')
            {
                const int spaces = tabSize - (column % tabSize);
                normalized.append(static_cast<std::size_t>(spaces), ' ');
                column += spaces;
            }
            else
            {
                normalized.push_back(ch);
                if (ch == '\n' || ch == '\r')
                    column = 0;
                else
                    ++column;
            }
        }

        return normalized;
    }

    std::string TrimTrailingSpacesPreserveNewlines(const std::string& text)
    {
        std::string normalized;
        normalized.reserve(text.size());

        std::size_t lineStart = 0;
        while (lineStart < text.size())
        {
            std::size_t lineEnd = lineStart;
            while (lineEnd < text.size() && text[lineEnd] != '\r' && text[lineEnd] != '\n')
                ++lineEnd;

            std::size_t trimmedEnd = lineEnd;
            while (trimmedEnd > lineStart && (text[trimmedEnd - 1] == ' ' || text[trimmedEnd - 1] == '\t'))
                --trimmedEnd;

            normalized.append(text, lineStart, trimmedEnd - lineStart);

            if (lineEnd < text.size())
            {
                if (text[lineEnd] == '\r')
                {
                    normalized.push_back('\r');
                    if (lineEnd + 1U < text.size() && text[lineEnd + 1U] == '\n')
                    {
                        normalized.push_back('\n');
                        lineStart = lineEnd + 2U;
                        continue;
                    }
                }
                else
                {
                    normalized.push_back('\n');
                }
            }

            lineStart = lineEnd + 1U;
        }

        return normalized;
    }

    void NormalizeDocumentWhitespace(DocumentState& document, int tabSize)
    {
        const auto originalText = document.editor->GetText();
        const auto normalizedText = NormalizeTabsToSpaces(originalText, tabSize);
        if (normalizedText == originalText)
            return;

        const auto cursor = document.editor->GetCursorPosition();
        document.editor->SetText(normalizedText);
        document.editor->SetCursorPosition(cursor);
        document.previewDirty = true;
        document.outlineDirty = true;
        ++document.outlineRevision;
        document.dirty = true;
    }

    void SyncEditorText(DocumentState& document, double currentTime)
    {
        if (document.editor->IsTextChanged())
        {
            const auto currentText = document.editor->GetText();
            if (currentText != document.lastSyncedText)
            {
                document.dirty = true;
                document.outlineDirty = true;
                ++document.outlineRevision;
                document.lastEditTime = currentTime;
                document.status = "Modified.";
                document.lastSyncedText = currentText;
            }
        }
    }

    void RefreshOutline(EditorState& state, DocumentState& document)
    {
        if (document.outlinePending && document.outlineTask.valid())
        {
            if (document.outlineTask.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
            {
                auto outline = document.outlineTask.get();
                document.outlinePending = false;
                if (document.outlineTaskRevision == document.outlineRevision)
                {
                    document.outline = std::move(outline);
                    document.outlineDirty = false;
                }
            }
        }

        if (!document.outlineDirty || document.outlinePending)
            return;

        std::optional<OutlineProjectContext> projectContext;
        if (state.project.has_value())
            projectContext = OutlineProjectContext{ state.project->rootDirectory };

        const auto documentPath = document.path;
        const auto documentText = document.editor->GetText();
        document.outlineTaskRevision = document.outlineRevision;
        document.outlinePending = true;
        document.outlineTask = std::async(
            std::launch::async,
            [projectContext = std::move(projectContext), documentPath, documentText]() {
                return AnalyzeOutline(projectContext, documentPath, documentText);
            });
    }

    AutoItPreprocessor::Compiler::CompilerOptions BuildCompilerOptions(const EditorState& state, const DocumentState& document)
    {
        AutoItPreprocessor::Compiler::CompilerOptions options;

        if (state.project.has_value())
        {
            for (const auto& path : SplitPaths(state.project->includeDirectories))
                options.includeDirectories.push_back(MakeAbsoluteProjectPath(*state.project, path));

            for (const auto& path : SplitPaths(state.project->customRuleFiles))
            {
                const auto absolutePath = MakeAbsoluteProjectPath(*state.project, path);
                if (std::filesystem::exists(absolutePath))
                    options.customRuleFiles.push_back(absolutePath);
            }
        }
        else
        {
            options.includeDirectories = SplitPaths(document.includeDirectories);
            for (const auto& path : SplitPaths(document.customRuleFiles))
            {
                if (std::filesystem::exists(path))
                    options.customRuleFiles.push_back(path);
            }
        }

        return options;
    }

    AutoItPreprocessor::Compiler::CompilationUnit RunCompilation(const EditorState& state, const DocumentState& document)
    {
        AutoItPreprocessor::Compiler::Compiler compiler;
        const auto options = BuildCompilerOptions(state, document);

        return compiler.Compile(
            AutoItPreprocessor::Common::SourceDocument{
                .path = document.path,
                .text = document.editor->GetText()
            },
            options);
    }

    void RefreshLivePreview(EditorState& state, DocumentState& document)
    {
        if (!document.previewDirty)
            return;

        if (!IsEditableProjectFile(document.path) || document.path.extension() == ".tokens" || document.path.extension() == kProjectExtension)
        {
            document.previewText.clear();
            document.previewStatus = "No preview for this file type.";
            document.previewDirty = false;
            return;
        }

        try
        {
            const auto compilation = RunCompilation(state, document);
            document.previewText = SanitizeUtf8Lossy(compilation.generatedCode);
            document.previewLineMappings = BuildPreviewLineMappings(compilation, document.path);
            if (document.previewEditor != nullptr)
            {
                document.previewEditor->SetText(document.previewText);
                document.previewEditor->SetReadOnly(true);
            }
            document.previewStatus = "Preview updated.";
            SyncPreviewHighlight(document, state.preferences);
        }
        catch (const std::exception& exception)
        {
            document.previewText.clear();
            document.previewLineMappings.clear();
            if (document.previewEditor != nullptr)
                document.previewEditor->SetText("");
            document.previewStatus = exception.what();
        }

        document.previewDirty = false;
    }

    void LoadDocumentFromPath(DocumentState& document, const std::filesystem::path& path, const EditorPreferences& preferences)
    {
        document.path = std::filesystem::absolute(path).lexically_normal();
        document.title = MakeDocumentTitle(document.path);
        document.sourceSyntax = DetermineSourceSyntax(document.path);
        document.editor->SetText(NormalizeTabsToSpaces(ReadTextFile(document.path)));
        ApplyDocumentPreferences(document, preferences);
        document.outputText.clear();
        document.tokenView.clear();
        document.previewText.clear();
        document.previewLineMappings.clear();
        document.previewStatus = "Build or preview to generate output.";
        if (document.previewEditor != nullptr)
        {
            document.previewEditor->SetText("");
        }
        document.previewDirty = false;
        document.outlineDirty = true;
        document.outlinePending = false;
        ++document.outlineRevision;
        document.status = "Loaded " + document.path.string();
        document.outputKind = OutputKind::None;
        document.dirty = false;
        document.lastSyncedText = document.editor->GetText();
        document.lastEditTime = 0.0;
    }

    void SyncPreviewHighlight(DocumentState& document, const EditorPreferences& preferences)
    {
        if (document.previewEditor != nullptr)
            document.previewEditor->ClearLineHighlights();

        if (document.editor == nullptr || document.previewEditor == nullptr || document.previewText.empty())
            return;

        auto selectionStart = document.editor->GetSelectionStart();
        auto selectionEnd = document.editor->GetSelectionEnd();
        if (selectionEnd < selectionStart)
            std::swap(selectionStart, selectionEnd);

        std::size_t sourceLineStart = static_cast<std::size_t>(selectionStart.mLine + 1);
        std::size_t sourceLineEnd = static_cast<std::size_t>(selectionEnd.mLine + 1);
        if (!document.editor->HasSelection())
        {
            sourceLineStart = static_cast<std::size_t>(document.editor->GetCursorPosition().mLine + 1);
            sourceLineEnd = sourceLineStart;
        }

        if (sourceLineStart >= document.previewLineMappings.size())
            return;

        sourceLineEnd = std::min(sourceLineEnd, document.previewLineMappings.size() - 1U);

        std::size_t generatedLineStart = 0;
        std::size_t generatedLineEnd = 0;
        for (std::size_t sourceLine = sourceLineStart; sourceLine <= sourceLineEnd; ++sourceLine)
        {
            const auto& mapping = document.previewLineMappings[sourceLine];
            if (mapping.generatedLineStart == 0 || mapping.generatedLineEnd == 0)
                continue;

            if (generatedLineStart == 0 || mapping.generatedLineStart < generatedLineStart)
                generatedLineStart = mapping.generatedLineStart;
            if (mapping.generatedLineEnd > generatedLineEnd)
                generatedLineEnd = mapping.generatedLineEnd;
        }

        if (generatedLineStart == 0 || generatedLineEnd == 0)
            return;

        const int previewStartLine = static_cast<int>(generatedLineStart - 1U);
        const int previewEndLine = static_cast<int>(generatedLineEnd - 1U);
        const int maxPreviewLine = std::max(0, document.previewEditor->GetTotalLines() - 1);
        const int clampedPreviewStartLine = std::clamp(previewStartLine, 0, maxPreviewLine);
        const int clampedPreviewEndLine = std::clamp(previewEndLine, clampedPreviewStartLine, maxPreviewLine);
        document.previewEditor->AddLineHighlight(clampedPreviewStartLine, clampedPreviewEndLine, ImGui::ColorConvertFloat4ToU32(preferences.previewMappingHighlight));
        if (!document.previewEditor->IsFocused())
            document.previewEditor->SetCursorPosition(TextEditor::Coordinates(clampedPreviewStartLine, 0));
    }

    void SaveDocument(DocumentState& document)
    {
        const auto originalText = document.editor->GetText();
        const auto text = TrimTrailingSpacesPreserveNewlines(NormalizeTabsToSpaces(originalText));
        if (text != originalText)
            document.editor->SetText(text);

        WriteTextFile(document.path, text);
        document.title = MakeDocumentTitle(document.path);
        document.status = "Saved " + document.path.string();
        document.dirty = false;
        document.lastSyncedText = text;
        document.previewDirty = false;
        document.outlineDirty = true;
    }

    void SaveDocumentAs(DocumentState& document)
    {
        if (const auto selectedPath = ShowFileDialog(true, "Torii Files\0*.aup;*.au3\0All Files\0*.*\0", document.path))
        {
            document.path = std::filesystem::absolute(*selectedPath).lexically_normal();
            SaveDocument(document);
        }
    }

    void PreviewTokens(EditorState& state, DocumentState& document)
    {
        const auto compilation = RunCompilation(state, document);
        document.tokenView = BuildTokenView(compilation.tokens);
        document.outputText.clear();
        document.outputKind = OutputKind::Tokens;
        document.status = "Tokenized " + std::to_string(compilation.tokens.size()) + " tokens.";
    }

    void PreviewStripped(EditorState& state, DocumentState& document)
    {
        const auto compilation = RunCompilation(state, document);
        document.outputText = compilation.strippedCode;
        document.tokenView.clear();
        document.outputKind = OutputKind::Stripped;
        document.status = "Prepared stripped preview.";
    }

    void PreviewCompiled(EditorState& state, DocumentState& document)
    {
        const auto compilation = RunCompilation(state, document);
        document.outputText = compilation.generatedCode;
        document.tokenView.clear();
        document.outputKind = OutputKind::Compiled;
        document.status = "Prepared compiled preview.";
    }

    std::filesystem::path MakeOutputPathForSave(const DocumentState& document)
    {
        const auto suffix = document.outputKind == OutputKind::Stripped ? "_stripped" : "_compiled";
        return MakeDerivedOutputPath(document.path, suffix);
    }

    void SaveDerivedOutput(DocumentState& document)
    {
        if (document.outputKind == OutputKind::None || document.outputKind == OutputKind::Tokens)
            throw std::runtime_error("There is no strip/build preview to save.");

        const auto outputPath = MakeOutputPathForSave(document);
        WriteTextFile(outputPath, document.outputText);
        document.status = "Saved output to " + outputPath.string();
    }

    void SaveDerivedOutputAs(DocumentState& document)
    {
        if (document.outputKind == OutputKind::None || document.outputKind == OutputKind::Tokens)
            throw std::runtime_error("There is no strip/build preview to save.");

        if (const auto selectedPath = ShowFileDialog(true, "AutoIt Output\0*.au3\0All Files\0*.*\0", MakeOutputPathForSave(document)))
        {
            WriteTextFile(*selectedPath, document.outputText);
            document.status = "Saved output to " + selectedPath->string();
        }
    }

    void SaveProject(const ProjectState& project)
    {
        auto projectCopy = project;
        projectCopy.includeDirectories = NormalizeProjectPathList(project, project.includeDirectories);
        projectCopy.customRuleFiles = NormalizeProjectPathList(project, project.customRuleFiles);
        Settings::ProjectSettingsFile(projectCopy, project.projectFilePath).Save();
    }

    ProjectState LoadProject(const std::filesystem::path& projectFilePath)
    {
        ProjectState project;
        Settings::ProjectSettingsFile settingsFile(project, projectFilePath);
        settingsFile.Load();

        if (Trim(project.customRuleFiles) == "rules.tokens"
            && !std::filesystem::exists(project.rootDirectory / "rules.tokens"))
        {
            project.customRuleFiles.clear();
        }

        return project;
    }

    ProjectState CreateNewProject(const std::filesystem::path& projectFilePath)
    {
        ProjectState project;
        const auto requestedPath = std::filesystem::absolute(projectFilePath).lexically_normal();
        project.name = requestedPath.stem().string();
        project.rootDirectory = requestedPath.parent_path() / project.name;
        project.projectFilePath = project.rootDirectory / (project.name + kProjectExtension);
        project.mainFilePath = GetProjectMainFilePath(project);
        project.debugOutputFilePath = GetProjectBuildOutputFilePath(project, BuildConfiguration::Debug);
        project.releaseOutputFilePath = GetProjectBuildOutputFilePath(project, BuildConfiguration::Release);
        project.includeDirectories.clear();
        project.customRuleFiles.clear();

        std::filesystem::create_directories(GetProjectCodeDirectory(project));
        std::filesystem::create_directories(project.rootDirectory / "build" / "debug");
        std::filesystem::create_directories(project.rootDirectory / "build" / "release");

        SaveProject(project);
        return project;
    }

    void LoadProjectIntoEditor(EditorState& state, const ProjectState& project)
    {
        state.project = project;
        state.documents.clear();
        state.currentDocumentIndex = 0;
        state.selectedProjectPath = GetProjectCodeDirectory(project);
        state.projectTree.clear();
        state.projectTreeRoot.clear();
        state.projectTreeLoading = false;
        LoadProjectWorkspace(state);
    }

    void SaveAllDocuments(EditorState& state)
    {
        for (auto& document : state.documents)
        {
            if (document.dirty)
                SaveDocument(document);
        }
    }

    std::optional<std::size_t> FindDocumentIndexByPath(const EditorState& state, const std::filesystem::path& path)
    {
        const auto normalized = std::filesystem::absolute(path).lexically_normal();
        for (std::size_t index = 0; index < state.documents.size(); ++index)
        {
            if (std::filesystem::absolute(state.documents[index].path).lexically_normal() == normalized)
                return index;
        }

        return std::nullopt;
    }

    void OpenDocumentInEditor(EditorState& state, const std::filesystem::path& path)
    {
        if (const auto existingIndex = FindDocumentIndexByPath(state, path))
        {
            state.currentDocumentIndex = *existingIndex;
            state.activateDocumentIndex = *existingIndex;
            state.requestFocusCurrentEditor = true;
            state.selectedProjectPath = std::filesystem::absolute(path).lexically_normal();
            SaveProjectWorkspace(state);
            return;
        }

        state.documents.push_back(MakeEmptyDocument(state.preferences));
        state.currentDocumentIndex = state.documents.size() - 1U;
        state.activateDocumentIndex = state.currentDocumentIndex;
        LoadDocumentFromPath(CurrentDocument(state), path, state.preferences);
        state.requestFocusCurrentEditor = true;
        state.selectedProjectPath = std::filesystem::absolute(path).lexically_normal();
        SaveProjectWorkspace(state);
    }

    std::filesystem::path EnsureUniquePath(const std::filesystem::path& desiredPath)
    {
        if (!std::filesystem::exists(desiredPath))
            return desiredPath;

        const auto parent = desiredPath.parent_path();
        const auto stem = desiredPath.stem().string();
        const auto extension = desiredPath.extension().string();

        for (int index = 2; index < 1000; ++index)
        {
            const auto candidate = parent / (stem + "_" + std::to_string(index) + extension);
            if (!std::filesystem::exists(candidate))
                return candidate;
        }

        throw std::runtime_error("Could not generate unique path for " + desiredPath.string());
    }

    void CopyPathRecursively(const std::filesystem::path& sourcePath, const std::filesystem::path& targetPath)
    {
        if (std::filesystem::is_directory(sourcePath))
        {
            std::filesystem::create_directories(targetPath);
            std::filesystem::copy(
                sourcePath,
                targetPath,
                std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
            return;
        }

        if (targetPath.has_parent_path())
            std::filesystem::create_directories(targetPath.parent_path());

        std::filesystem::copy_file(sourcePath, targetPath, std::filesystem::copy_options::overwrite_existing);
    }

    void MoveProjectPath(EditorState& state, const std::filesystem::path& sourcePath, const std::filesystem::path& targetDirectory)
    {
        const auto targetPath = targetDirectory / sourcePath.filename();
        if (std::filesystem::equivalent(sourcePath.parent_path(), targetDirectory))
            return;

        std::filesystem::create_directories(targetDirectory);
        std::filesystem::rename(sourcePath, targetPath);

        if (const auto existingIndex = FindDocumentIndexByPath(state, sourcePath))
        {
                    state.documents[*existingIndex].path = targetPath;
                    state.documents[*existingIndex].title = MakeDocumentTitle(targetPath);
                    state.documents[*existingIndex].previewDirty = true;
                    state.documents[*existingIndex].outlineDirty = true;
                }

        if (state.project.has_value() && state.project->mainFilePath == sourcePath)
            state.project->mainFilePath = targetPath;
    }

    std::filesystem::path GetProjectStateDirectory(const ProjectState& project)
    {
        return project.rootDirectory / ".autoit";
    }

    void EnsureProjectStateDirectory(const ProjectState& project)
    {
        const auto stateDirectory = GetProjectStateDirectory(project);
        std::filesystem::create_directories(stateDirectory);
#if defined(_WIN32)
        MarkDirectoryHidden(stateDirectory);
#endif
    }

    void LoadProjectWorkspace(EditorState& state)
    {
        if (!state.project.has_value())
            return;

        EnsureProjectStateDirectory(*state.project);
        const auto workspacePath = GetProjectStateDirectory(*state.project) / "workspace.json";
        const auto legacyWorkspacePath = GetProjectStateDirectory(*state.project) / "workspace.ini";
        if (!std::filesystem::exists(workspacePath) && !std::filesystem::exists(legacyWorkspacePath))
            return;

        Settings::WorkspaceSessionData workspaceData;
        workspaceData.buildConfiguration = state.buildConfiguration;
        const auto pathToRead = std::filesystem::exists(workspacePath) ? workspacePath : legacyWorkspacePath;
        Settings::WorkspaceSettingsFile(*state.project, workspaceData, pathToRead).Load();

        state.buildConfiguration = workspaceData.buildConfiguration;
        if (workspaceData.openFiles.empty())
            return;

        state.documents.clear();
        for (const auto& path : workspaceData.openFiles)
        {
            if (!std::filesystem::exists(path))
                continue;

            state.documents.push_back(MakeEmptyDocument(state.preferences));
            LoadDocumentFromPath(state.documents.back(), path, state.preferences);
        }

        if (!state.documents.empty())
        {
            state.currentDocumentIndex = std::min(workspaceData.currentIndex, state.documents.size() - 1U);
            state.activateDocumentIndex = state.currentDocumentIndex;
            state.selectedProjectPath = state.documents[state.currentDocumentIndex].path;
        }
    }

    void SaveProjectWorkspace(const EditorState& state)
    {
        if (!state.project.has_value())
            return;

        EnsureProjectStateDirectory(*state.project);
        const auto workspacePath = GetProjectStateDirectory(*state.project) / "workspace.json";
        Settings::WorkspaceSessionData workspaceData;
        workspaceData.buildConfiguration = state.buildConfiguration;
        workspaceData.currentIndex = state.currentDocumentIndex;
        for (const auto& document : state.documents)
        {
            if (document.path.empty())
                continue;
            workspaceData.openFiles.push_back(document.path);
        }
        Settings::WorkspaceSettingsFile(*state.project, workspaceData, workspacePath).Save();
    }

    std::filesystem::path GetGlobalEditorSettingsPath()
    {
        return Settings::UserSettingsFile::DefaultPath();
    }

    void LoadEditorPreferences(EditorPreferences& preferences)
    {
        Settings::UserSettingsFile(preferences).Load();
    }

    void SaveEditorPreferences(const EditorPreferences& preferences)
    {
        auto preferencesCopy = preferences;
        Settings::UserSettingsFile(preferencesCopy).Save();
    }

    std::filesystem::path GetGlobalShortcutSettingsPath()
    {
        return Settings::ShortcutSettingsFile::DefaultPath();
    }

    void LoadShortcutOverrides(std::unordered_map<std::string, std::string>& shortcuts)
    {
        Settings::ShortcutSettingsFile(shortcuts).Load();
    }

    void SaveShortcutOverrides(const std::unordered_map<std::string, std::string>& shortcuts)
    {
        auto shortcutsCopy = shortcuts;
        Settings::ShortcutSettingsFile(shortcutsCopy).Save();
    }

    const std::filesystem::path& GetProjectBuildOutputPath(const ProjectState& project, BuildConfiguration configuration)
    {
        return configuration == BuildConfiguration::Release ? project.releaseOutputFilePath : project.debugOutputFilePath;
    }

    const char* BuildConfigurationLabel(BuildConfiguration configuration)
    {
        return configuration == BuildConfiguration::Release ? "Release" : "Debug";
    }

    void BuildProject(EditorState& state)
    {
        if (!state.project.has_value())
            throw std::runtime_error("No project loaded.");

        SaveAllDocuments(state);
        SaveProject(*state.project);

        AutoItPreprocessor::Compiler::Compiler compiler;
        const DocumentState fallbackDocument{};
        const auto options = BuildCompilerOptions(state, HasOpenDocument(state) ? CurrentDocument(state) : fallbackDocument);
        const auto compilation = compiler.Compile(state.project->mainFilePath, options);
        const auto& outputPath = GetProjectBuildOutputPath(*state.project, state.buildConfiguration);
        const auto generatedCode = SanitizeUtf8Lossy(compilation.generatedCode);
        WriteTextFile(outputPath, generatedCode);

        if (HasOpenDocument(state))
        {
            auto& document = CurrentDocument(state);
            document.outputText = generatedCode;
            document.outputKind = OutputKind::Compiled;
            document.previewText = generatedCode;
            document.previewLineMappings = BuildPreviewLineMappings(compilation, document.path);
            if (document.previewEditor != nullptr)
                document.previewEditor->SetText(document.previewText);
            document.previewStatus = "Build updated preview.";
            document.status = "Built " + outputPath.string() + " [" + BuildConfigurationLabel(state.buildConfiguration) + "]";
            SyncPreviewHighlight(document, state.preferences);
        }
    }

    void BuildCurrentDocument(EditorState& state)
    {
        if (state.project.has_value())
        {
            BuildProject(state);
            return;
        }

        auto& document = CurrentDocument(state);
        const auto compilation = RunCompilation(state, document);
        document.outputText = SanitizeUtf8Lossy(compilation.generatedCode);
        document.outputKind = OutputKind::Compiled;
        document.tokenView.clear();
        document.previewText = document.outputText;
        document.previewLineMappings = BuildPreviewLineMappings(compilation, document.path);
        if (document.previewEditor != nullptr)
            document.previewEditor->SetText(document.previewText);
        document.previewStatus = "Build updated preview.";
        document.status = "Built preview for " + document.title;
        SyncPreviewHighlight(document, state.preferences);
    }

    void RunBuiltProject(EditorState& state)
    {
#if defined(_WIN32)
        if (!state.project.has_value())
            throw std::runtime_error("Run requires a loaded project.");

        if (state.runInProgress)
            throw std::runtime_error("Run already in progress.");

        BuildProject(state);

        const auto interpreterPath = FindAutoItInterpreterPath();
        if (!interpreterPath.has_value())
            throw std::runtime_error("Could not locate AutoIt3.exe in the registry.");
        const auto& outputPath = GetProjectBuildOutputPath(*state.project, state.buildConfiguration);
        const auto rootDirectory = state.project->rootDirectory;
        const auto buildLabel = std::string(BuildConfigurationLabel(state.buildConfiguration));
        const std::string commandLineUtf8 = "\"" + interpreterPath->string() + "\" /ErrorStdOut \"" + outputPath.string() + "\"";
        state.runOutput = "> " + commandLineUtf8 + "\n";
        state.runStatus = "Running [" + buildLabel + "]...";
        state.liveRunOutput = std::make_shared<RunOutputBuffer>();
        state.liveRunOutput->output = state.runOutput;
        if (state.runEditor != nullptr)
            SetLoggerText(*state.runEditor, state.runOutput);
        state.showRun = true;
        state.activeBottomTab = BottomPanelTab::Run;
        state.requestedBottomTab = BottomPanelTab::Run;
        if (HasOpenDocument(state))
            CurrentDocument(state).status = state.runStatus;
        state.runInProgress = true;
        state.runTask = std::async(std::launch::async, [interpreter = *interpreterPath, outputPath, rootDirectory, buildLabel, commandLineUtf8, liveRunOutput = state.liveRunOutput]() -> RunTaskResult {
            SECURITY_ATTRIBUTES securityAttributes{};
            securityAttributes.nLength = sizeof(securityAttributes);
            securityAttributes.bInheritHandle = TRUE;

            HANDLE stdoutRead = nullptr;
            HANDLE stdoutWrite = nullptr;
            HANDLE stderrRead = nullptr;
            HANDLE stderrWrite = nullptr;
            if (!CreatePipe(&stdoutRead, &stdoutWrite, &securityAttributes, 0))
                throw std::runtime_error("Failed to create stdout pipe.");
            if (!CreatePipe(&stderrRead, &stderrWrite, &securityAttributes, 0))
            {
                CloseHandle(stdoutRead);
                CloseHandle(stdoutWrite);
                throw std::runtime_error("Failed to create stderr pipe.");
            }

            SetHandleInformation(stdoutRead, HANDLE_FLAG_INHERIT, 0);
            SetHandleInformation(stderrRead, HANDLE_FLAG_INHERIT, 0);

            STARTUPINFOW startupInfo{};
            startupInfo.cb = sizeof(startupInfo);
            startupInfo.dwFlags = STARTF_USESTDHANDLES;
            startupInfo.hStdOutput = stdoutWrite;
            startupInfo.hStdError = stderrWrite;
            startupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

            PROCESS_INFORMATION processInfo{};
            std::wstring commandLine = L"AutoIt3.exe /ErrorStdOut \"" + outputPath.wstring() + L"\"";
            std::string output = "> " + commandLineUtf8 + "\n";

            const BOOL created = CreateProcessW(
                interpreter.wstring().c_str(),
                commandLine.data(),
                nullptr,
                nullptr,
                TRUE,
                CREATE_NO_WINDOW,
                nullptr,
                rootDirectory.wstring().c_str(),
                &startupInfo,
                &processInfo);

            CloseHandle(stdoutWrite);
            CloseHandle(stderrWrite);

            if (!created)
            {
                const DWORD errorCode = GetLastError();
                CloseHandle(stdoutRead);
                CloseHandle(stderrRead);
                throw std::runtime_error(
                    "Failed to launch " + interpreter.string() +
                    " for " + outputPath.string() +
                    " in " + rootDirectory.string() +
                    " (Win32 " + std::to_string(errorCode) + ": " + DescribeWindowsError(errorCode) + ").");
            }

            std::array<char, 4096> buffer{};
            auto appendOutput = [&](HANDLE pipeHandle, bool isError) {
                DWORD availableBytes = 0;
                if (!PeekNamedPipe(pipeHandle, nullptr, 0, nullptr, &availableBytes, nullptr) || availableBytes == 0)
                    return;

                DWORD bytesRead = 0;
                if (!ReadFile(pipeHandle, buffer.data(), static_cast<DWORD>(std::min<std::size_t>(buffer.size(), availableBytes)), &bytesRead, nullptr) || bytesRead == 0)
                    return;

                if (isError)
                {
                    const std::string raw(buffer.data(), buffer.data() + bytesRead);
                    std::size_t start = 0;
                    while (start < raw.size())
                    {
                        const std::size_t end = raw.find('\n', start);
                        const std::string_view line(raw.data() + start, (end == std::string::npos ? raw.size() : end) - start);
                        output += "[STDERR] ";
                        output.append(line.data(), line.size());
                        output.push_back('\n');
                        if (end == std::string::npos)
                            break;
                        start = end + 1U;
                    }
                }
                else
                {
                    output.append(buffer.data(), buffer.data() + bytesRead);
                }

                if (liveRunOutput != nullptr)
                {
                    std::scoped_lock lock(liveRunOutput->mutex);
                    liveRunOutput->output = output;
                }
            };

            for (;;)
            {
                appendOutput(stdoutRead, false);
                appendOutput(stderrRead, true);

                const DWORD waitResult = WaitForSingleObject(processInfo.hProcess, 20);
                if (waitResult == WAIT_OBJECT_0)
                    break;
            }

            DWORD bytesRead = 0;
            while (ReadFile(stdoutRead, buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead, nullptr) && bytesRead > 0)
            {
                output.append(buffer.data(), buffer.data() + bytesRead);
                if (liveRunOutput != nullptr)
                {
                    std::scoped_lock lock(liveRunOutput->mutex);
                    liveRunOutput->output = output;
                }
            }

            while (ReadFile(stderrRead, buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead, nullptr) && bytesRead > 0)
            {
                const std::string raw(buffer.data(), buffer.data() + bytesRead);
                std::size_t start = 0;
                while (start < raw.size())
                {
                    const std::size_t end = raw.find('\n', start);
                    const std::string_view line(raw.data() + start, (end == std::string::npos ? raw.size() : end) - start);
                    output += "[STDERR] ";
                    output.append(line.data(), line.size());
                    output.push_back('\n');
                    if (end == std::string::npos)
                        break;
                    start = end + 1U;
                }
                if (liveRunOutput != nullptr)
                {
                    std::scoped_lock lock(liveRunOutput->mutex);
                    liveRunOutput->output = output;
                }
            }

            DWORD exitCode = 0;
            GetExitCodeProcess(processInfo.hProcess, &exitCode);

            CloseHandle(stdoutRead);
            CloseHandle(stderrRead);
            CloseHandle(processInfo.hThread);
            CloseHandle(processInfo.hProcess);

            if (output.back() != '\n')
                output.push_back('\n');
            output += "Finished with exit code " + std::to_string(exitCode) + "\n";

            if (liveRunOutput != nullptr)
            {
                std::scoped_lock lock(liveRunOutput->mutex);
                liveRunOutput->output = output;
            }

            return RunTaskResult{
                output,
                "Run finished with exit code " + std::to_string(exitCode) + " [" + buildLabel + "].",
                static_cast<int>(exitCode)
            };
        });
#else
        (void)state;
        throw std::runtime_error("Run is currently only implemented on Windows.");
#endif
    }

    void PollRunTask(EditorState& state)
    {
        if (!state.runInProgress)
            return;

        if (state.liveRunOutput != nullptr)
        {
            std::string liveOutput;
            {
                std::scoped_lock lock(state.liveRunOutput->mutex);
                liveOutput = state.liveRunOutput->output;
            }

            if (!liveOutput.empty() && liveOutput != state.runOutput)
            {
                state.runOutput = liveOutput;
                if (state.runEditor != nullptr)
                    SetLoggerText(*state.runEditor, state.runOutput);
            }
        }

        if (!state.runTask.valid())
        {
            state.liveRunOutput.reset();
            state.runInProgress = false;
            return;
        }

        if (state.runTask.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
            return;

        try
        {
            const auto result = state.runTask.get();
            state.runOutput = result.output;
            state.runStatus = result.status;
            state.liveRunOutput.reset();
            if (state.runEditor != nullptr)
                SetLoggerText(*state.runEditor, state.runOutput);
            if (HasOpenDocument(state))
                CurrentDocument(state).status = state.runStatus;
        }
        catch (const std::exception& exception)
        {
            state.runOutput = "[ERROR] " + std::string(exception.what()) + "\n";
            state.runStatus = "Run failed.";
            state.liveRunOutput.reset();
            if (state.runEditor != nullptr)
                SetLoggerText(*state.runEditor, state.runOutput);
            if (HasOpenDocument(state))
                CurrentDocument(state).status = exception.what();
        }

        state.showRun = true;
        state.activeBottomTab = BottomPanelTab::Run;
        state.requestedBottomTab = BottomPanelTab::Run;
        state.runInProgress = false;
    }

    void CreateNewDocument(EditorState& state)
    {
        state.documents.push_back(MakeEmptyDocument(state.preferences));
        state.currentDocumentIndex = state.documents.size() - 1U;
        state.activateDocumentIndex = state.currentDocumentIndex;
        ApplyDocumentPreferences(CurrentDocument(state), state.preferences);
        state.requestFocusCurrentEditor = true;
        SaveProjectWorkspace(state);
    }

    void CloseDocument(EditorState& state, std::size_t documentIndex)
    {
        if (state.documents.size() == 1U)
        {
            state.documents.clear();
            state.currentDocumentIndex = 0;
            state.activateDocumentIndex.reset();
            SaveProjectWorkspace(state);
            return;
        }

        state.documents.erase(state.documents.begin() + static_cast<std::ptrdiff_t>(documentIndex));
        if (state.currentDocumentIndex >= state.documents.size())
            state.currentDocumentIndex = state.documents.size() - 1U;
        state.activateDocumentIndex.reset();
        SaveProjectWorkspace(state);
    }

    void RequestAction(EditorState& state, PendingAction::Kind kind, std::size_t documentIndex)
    {
        state.pendingAction = PendingAction{kind, documentIndex};
        ImGui::OpenPopup("Unsaved Changes");
    }

    void ExecutePendingAction(EditorState& state)
    {
        switch (state.pendingAction.kind)
        {
            case PendingAction::Kind::NewDocument:
                CreateNewDocument(state);
                break;
            case PendingAction::Kind::OpenDocument:
            {
                const auto basePath = state.documents.empty() ? std::filesystem::path{} : CurrentDocument(state).path;
                if (const auto selectedPath = ShowFileDialog(false, "Torii Files\0*.aup;*.au3\0All Files\0*.*\0", basePath))
                    OpenDocumentInEditor(state, *selectedPath);
                break;
            }
            case PendingAction::Kind::CloseDocument:
                CloseDocument(state, state.pendingAction.documentIndex);
                break;
            case PendingAction::Kind::ExitApplication:
                SaveProjectWorkspace(state);
                state.requestExit = true;
                break;
            case PendingAction::Kind::None:
                break;
        }

        state.pendingAction = {};
    }

    const char* OutputKindLabel(OutputKind kind)
    {
        switch (kind)
        {
            case OutputKind::None:
                return "No Preview";
            case OutputKind::Tokens:
                return "Tokens";
            case OutputKind::Stripped:
                return "Stripped";
            case OutputKind::Compiled:
                return "Built";
        }

        return "Unknown";
    }
}

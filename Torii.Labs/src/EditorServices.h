#pragma once

#include "EditorState.h"

#include "AutoItPreprocessor/Compiler/Compiler.h"
#include "AutoItPreprocessor/Tokenizer/Token.h"

#include <optional>
#include <string>
#include <vector>

namespace AutoItPlus::Editor
{
    std::string ReadTextFile(const std::filesystem::path& path);
    void WriteTextFile(const std::filesystem::path& path, const std::string& text);

    std::vector<std::filesystem::path> SplitPaths(const std::string& raw);
    std::string JoinPaths(const std::vector<std::filesystem::path>& paths);
    std::filesystem::path MakeAbsoluteProjectPath(const ProjectState& project, const std::filesystem::path& path);
    std::filesystem::path MakeProjectRelativePath(const ProjectState& project, const std::filesystem::path& path);
    std::string NormalizeProjectPathList(const ProjectState& project, const std::string& raw);
    bool IsEditableProjectFile(const std::filesystem::path& path);

    std::optional<std::filesystem::path> ShowFileDialog(
        bool saveDialog,
        const char* filter,
        const std::filesystem::path& initialPath = {});

    std::filesystem::path MakeDerivedOutputPath(const std::filesystem::path& inputPath, const std::string& suffix);
    std::string BuildTokenView(const std::vector<AutoItPreprocessor::Tokenizer::Token>& tokens);
    std::string MakeDocumentTitle(const std::filesystem::path& path);
    DocumentState MakeEmptyDocument(const EditorPreferences& preferences);
    std::string NormalizeTabsToSpaces(const std::string& text, int tabSize = 4);
    void NormalizeDocumentWhitespace(DocumentState& document, int tabSize = 4);
    void SyncEditorText(DocumentState& document, double currentTime);
    void RefreshOutline(EditorState& state, DocumentState& document);

    AutoItPreprocessor::Compiler::CompilerOptions BuildCompilerOptions(const EditorState& state, const DocumentState& document);
    AutoItPreprocessor::Compiler::CompilationUnit RunCompilation(const EditorState& state, const DocumentState& document);
    void RefreshLivePreview(EditorState& state, DocumentState& document);
    void SyncPreviewHighlight(DocumentState& document, const EditorPreferences& preferences);

    void LoadDocumentFromPath(DocumentState& document, const std::filesystem::path& path, const EditorPreferences& preferences);
    void SaveDocument(DocumentState& document);
    void SaveDocumentAs(DocumentState& document);
    void PreviewTokens(EditorState& state, DocumentState& document);
    void PreviewStripped(EditorState& state, DocumentState& document);
    void PreviewCompiled(EditorState& state, DocumentState& document);
    std::filesystem::path MakeOutputPathForSave(const DocumentState& document);
    void SaveDerivedOutput(DocumentState& document);
    void SaveDerivedOutputAs(DocumentState& document);

    void SaveProject(const ProjectState& project);
    ProjectState LoadProject(const std::filesystem::path& projectFilePath);
    ProjectState CreateNewProject(const std::filesystem::path& projectFilePath);
    void LoadProjectIntoEditor(EditorState& state, const ProjectState& project);

    void SaveAllDocuments(EditorState& state);
    std::optional<std::size_t> FindDocumentIndexByPath(const EditorState& state, const std::filesystem::path& path);
    void OpenDocumentInEditor(EditorState& state, const std::filesystem::path& path);
    std::filesystem::path EnsureUniquePath(const std::filesystem::path& desiredPath);
    void CopyPathRecursively(const std::filesystem::path& sourcePath, const std::filesystem::path& targetPath);
    void MoveProjectPath(EditorState& state, const std::filesystem::path& sourcePath, const std::filesystem::path& targetDirectory);
    std::filesystem::path GetProjectStateDirectory(const ProjectState& project);
    void EnsureProjectStateDirectory(const ProjectState& project);
    void LoadProjectWorkspace(EditorState& state);
    void SaveProjectWorkspace(const EditorState& state);
    const std::filesystem::path& GetProjectBuildOutputPath(const ProjectState& project, BuildConfiguration configuration);
    const char* BuildConfigurationLabel(BuildConfiguration configuration);
    std::filesystem::path GetGlobalEditorSettingsPath();
    void LoadEditorPreferences(EditorPreferences& preferences);
    void SaveEditorPreferences(const EditorPreferences& preferences);
    std::filesystem::path GetGlobalShortcutSettingsPath();
    void LoadShortcutOverrides(std::unordered_map<std::string, std::string>& shortcuts);
    void SaveShortcutOverrides(const std::unordered_map<std::string, std::string>& shortcuts);

    void BuildProject(EditorState& state);
    void BuildCurrentDocument(EditorState& state);
    void RunBuiltProject(EditorState& state);
    void PollRunTask(EditorState& state);

    void CreateNewDocument(EditorState& state);
    void CloseDocument(EditorState& state, std::size_t documentIndex);
    void RequestAction(EditorState& state, PendingAction::Kind kind, std::size_t documentIndex = 0);
    void ExecutePendingAction(EditorState& state);

    const char* OutputKindLabel(OutputKind kind);
}

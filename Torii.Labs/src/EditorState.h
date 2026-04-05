#pragma once

#include "ConsoleWidget.h"
#include "HotkeyManager.h"
#include "SymbolAnalysis.h"
#include "TextEditor.h"
#include "AutoItPreprocessor/Compiler/Compiler.h"
#include "imgui.h"

#include <cstddef>
#include <filesystem>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace AutoItPlus::Editor
{
    enum class SyntaxFlavor
    {
        AutoIt,
        AutoItPlus,
        Logger
    };

    enum class ThemePreset
    {
        Custom,
        Torii,
        Umi,
        Midnight,
        Forest,
        Daylight
    };

    enum class ZoomTarget
    {
        Source,
        Preview,
        Output
    };

    struct SyntaxPaletteSettings
    {
        ImVec4 keyword = ImVec4(0.84f, 0.68f, 0.26f, 1.0f);
        ImVec4 identifier = ImVec4(0.84f, 0.86f, 0.90f, 1.0f);
        ImVec4 knownIdentifier = ImVec4(0.48f, 0.78f, 0.64f, 1.0f);
        ImVec4 preprocessor = ImVec4(0.43f, 0.67f, 0.91f, 1.0f);
        ImVec4 comment = ImVec4(0.48f, 0.53f, 0.60f, 1.0f);
        ImVec4 string = ImVec4(0.83f, 0.52f, 0.35f, 1.0f);
        ImVec4 number = ImVec4(0.73f, 0.61f, 0.91f, 1.0f);
        ImVec4 punctuation = ImVec4(0.74f, 0.78f, 0.83f, 1.0f);
        ImVec4 background = ImVec4(0.06f, 0.07f, 0.09f, 1.0f);
        ImVec4 currentLine = ImVec4(0.14f, 0.17f, 0.22f, 0.42f);
        ImVec4 lineNumber = ImVec4(0.62f, 0.69f, 0.77f, 1.0f);
        ImVec4 selection = ImVec4(0.20f, 0.36f, 0.55f, 0.45f);
        ImVec4 currentLineEdge = ImVec4(0.29f, 0.47f, 0.67f, 0.9f);
    };

    struct UiThemeSettings
    {
        ImVec4 accentColor = ImVec4(0.20f, 0.55f, 0.93f, 1.0f);
        ImVec4 accentSoftColor = ImVec4(0.17f, 0.27f, 0.38f, 1.0f);
        ImVec4 panelColor = ImVec4(0.09f, 0.11f, 0.15f, 1.0f);
        ImVec4 panelAltColor = ImVec4(0.11f, 0.13f, 0.18f, 1.0f);
        ImVec4 windowBg = ImVec4(0.05f, 0.06f, 0.09f, 1.0f);
        ImVec4 menuBarBg = ImVec4(0.08f, 0.10f, 0.14f, 1.0f);
        ImVec4 borderColor = ImVec4(0.16f, 0.21f, 0.29f, 1.0f);
        ImVec4 headerHovered = ImVec4(0.24f, 0.39f, 0.58f, 1.0f);
        ImVec4 buttonHovered = ImVec4(0.25f, 0.43f, 0.63f, 1.0f);
        ImVec4 buttonActive = ImVec4(0.18f, 0.34f, 0.51f, 1.0f);
        ImVec4 frameBg = ImVec4(0.10f, 0.12f, 0.17f, 1.0f);
        ImVec4 frameBgHovered = ImVec4(0.14f, 0.18f, 0.25f, 1.0f);
        ImVec4 tabColor = ImVec4(0.11f, 0.14f, 0.19f, 1.0f);
        ImVec4 tabHovered = ImVec4(0.19f, 0.29f, 0.42f, 1.0f);
        ImVec4 titleBg = ImVec4(0.09f, 0.11f, 0.15f, 1.0f);
        ImVec4 titleBgActive = ImVec4(0.11f, 0.14f, 0.19f, 1.0f);
        ImVec4 separatorColor = ImVec4(0.16f, 0.21f, 0.29f, 1.0f);
        ImVec4 resizeGrip = ImVec4(0.18f, 0.31f, 0.47f, 0.5f);
        ImVec4 sliderGrabActive = ImVec4(0.31f, 0.65f, 1.0f, 1.0f);
        ImVec4 textColor = ImVec4(0.88f, 0.92f, 0.97f, 1.0f);
        ImVec4 textDisabledColor = ImVec4(0.56f, 0.63f, 0.73f, 1.0f);
        ImVec4 popupBg = ImVec4(0.08f, 0.10f, 0.14f, 1.0f);
        ImVec4 iconPrimary = ImVec4(0.59f, 0.82f, 1.0f, 1.0f);
        ImVec4 iconSecondary = ImVec4(0.96f, 0.76f, 0.36f, 1.0f);
        ImVec4 iconNeutral = ImVec4(0.82f, 0.86f, 0.93f, 1.0f);
        ImVec4 iconSuccess = ImVec4(0.40f, 0.86f, 0.64f, 1.0f);
        ImVec4 directoryDropHighlight = ImVec4(0.59f, 0.82f, 1.0f, 0.18f);
        ImVec4 fileDropLine = ImVec4(0.59f, 0.82f, 1.0f, 0.95f);
    };

    struct EditorPreferences
    {
        SyntaxPaletteSettings autoIt = {};
        SyntaxPaletteSettings autoItPlus = {
            ImVec4(0.92f, 0.72f, 0.24f, 1.0f),
            ImVec4(0.89f, 0.90f, 0.94f, 1.0f),
            ImVec4(0.51f, 0.84f, 0.70f, 1.0f),
            ImVec4(0.35f, 0.78f, 0.98f, 1.0f),
            ImVec4(0.48f, 0.53f, 0.60f, 1.0f),
            ImVec4(0.95f, 0.58f, 0.37f, 1.0f),
            ImVec4(0.76f, 0.64f, 0.95f, 1.0f),
            ImVec4(0.80f, 0.84f, 0.90f, 1.0f),
            ImVec4(0.05f, 0.07f, 0.10f, 1.0f),
            ImVec4(0.15f, 0.20f, 0.26f, 0.42f),
            ImVec4(0.62f, 0.69f, 0.77f, 1.0f),
            ImVec4(0.20f, 0.36f, 0.55f, 0.45f),
            ImVec4(0.29f, 0.47f, 0.67f, 0.9f)
        };
        UiThemeSettings uiTheme = {};
        ImVec4 previewMappingHighlight = ImVec4(0.28f, 0.46f, 0.77f, 0.28f);
        float editorZoom = 1.0f;
        float previewZoom = 1.0f;
        float outputZoom = 1.0f;
        bool showWhitespace = false;
        bool showLineNumbers = true;
        bool showPreferences = false;
        ThemePreset themePreset = ThemePreset::Torii;
    };

    enum class OutputKind
    {
        None,
        Tokens,
        Stripped,
        Compiled
    };

    enum class BuildConfiguration
    {
        Debug,
        Release
    };

    enum class BottomPanelTab
    {
        Output,
        Run
    };

    struct PendingAction
    {
        enum class Kind
        {
            None,
            NewDocument,
            OpenDocument,
            CloseDocument,
            ExitApplication
        };

        Kind kind = Kind::None;
        std::size_t documentIndex = 0;
    };

    struct DocumentState
    {
        struct PreviewLineMapping
        {
            std::size_t generatedLineStart = 0;
            std::size_t generatedLineEnd = 0;
            bool isIncludeExpansion = false;
            bool isSkippedIncludeExpansion = false;
        };

        std::string title = "Untitled.aup";
        std::filesystem::path path = std::filesystem::current_path() / "Untitled.aup";
        std::string includeDirectories;
        std::string customRuleFiles;
        std::string outputText;
        std::string tokenView;
        std::string previewText;
        std::string previewStatus = "No preview.";
        std::string status = "Ready.";
        std::string lastSyncedText;
        std::vector<PreviewLineMapping> previewLineMappings;
        OutputKind outputKind = OutputKind::None;
        SyntaxFlavor sourceSyntax = SyntaxFlavor::AutoItPlus;
        std::unique_ptr<TextEditor> editor;
        std::unique_ptr<TextEditor> previewEditor;
        bool dirty = false;
        bool showWhitespace = false;
        bool previewDirty = true;
        bool outlineDirty = true;
        bool outlinePending = false;
        std::uint64_t outlineRevision = 0;
        std::uint64_t outlineTaskRevision = 0;
        double lastEditTime = 0.0;
        OutlineData outline;
        std::future<OutlineData> outlineTask;
    };

    struct ProjectState
    {
        std::string name;
        std::filesystem::path projectFilePath;
        std::filesystem::path rootDirectory;
        std::filesystem::path mainFilePath;
        std::filesystem::path debugOutputFilePath;
        std::filesystem::path releaseOutputFilePath;
        std::string includeDirectories;
        std::string customRuleFiles;
    };

    struct ProjectSettingsDialogState
    {
        bool show = false;
        std::string mainFilePath;
    };

    struct ProjectTreeNode
    {
        std::filesystem::path path;
        std::string name;
        bool isDirectory = false;
        std::vector<ProjectTreeNode> children;
    };

    struct ProjectTreeScopeRect
    {
        ImVec2 min = ImVec2(0.0f, 0.0f);
        ImVec2 max = ImVec2(0.0f, 0.0f);
    };

    struct FileActionState
    {
        enum class Kind
        {
            None,
            NewFile,
            NewFolder,
            Rename,
            Copy,
            Duplicate,
            Delete
        };

        Kind kind = Kind::None;
        std::filesystem::path sourcePath;
        std::filesystem::path baseDirectory;
        std::string inputPath;
        bool openAfterCreate = false;
        bool openRequested = false;
    };

    struct RunTaskResult
    {
        std::string output;
        std::string status;
        int exitCode = 0;
    };

    struct BuildTaskResult
    {
        bool succeeded = false;
        bool projectBuild = false;
        std::filesystem::path documentPath;
        std::filesystem::path outputPath;
        std::string previewText;
        std::string status;
        std::string error;
        AutoItPreprocessor::Compiler::CompilationUnit compilation;
    };

    struct RunOutputBuffer
    {
        std::mutex mutex;
        std::string output;
    };

    struct EditorState
    {
        std::vector<DocumentState> documents;
        std::size_t currentDocumentIndex = 0;
        std::optional<std::size_t> activateDocumentIndex;
        std::optional<ProjectState> project;
        FileActionState fileAction = {};
        PendingAction pendingAction = {};
        bool requestExit = false;
        bool showOutput = true;
        bool showTokens = true;
        bool showInspector = true;
        bool showOutline = true;
        bool showRun = true;
        float sidebarWidth = 280.0f;
        float outlineWidth = 260.0f;
        float bottomPanelHeight = 200.0f;
        float previewWidth = 420.0f;
        std::string runOutput;
        std::string runStatus = "Idle.";
        std::string outputLog;
        std::unique_ptr<ConsoleWidget> outputEditor;
        std::unique_ptr<ConsoleWidget> runEditor;
        std::optional<std::filesystem::path> selectedProjectPath;
        std::optional<std::filesystem::path> requestedOpenPath;
        bool requestFocusCurrentEditor = false;
        EditorPreferences preferences;
        ProjectSettingsDialogState projectSettingsDialog;
        std::vector<ProjectTreeNode> projectTree;
        std::future<std::vector<ProjectTreeNode>> projectTreeTask;
        std::filesystem::path projectTreeRoot;
        std::unordered_set<std::string> expandedProjectDirectories;
        std::optional<std::filesystem::path> projectTreeDropPreviewDirectory;
        std::unordered_map<std::string, ProjectTreeScopeRect> projectTreeScopeRects;
        bool projectTreeLoading = false;
        std::unordered_map<std::string, std::string> shortcutOverrides;
        std::unordered_map<std::string, std::string> shortcutEditBuffers;
        HotkeyManager hotkeys;
        ZoomTarget activeZoomTarget = ZoomTarget::Source;
        BuildConfiguration buildConfiguration = BuildConfiguration::Debug;
        BottomPanelTab activeBottomTab = BottomPanelTab::Output;
        std::optional<BottomPanelTab> requestedBottomTab;
        std::future<BuildTaskResult> buildTask;
        bool buildInProgress = false;
        bool runAfterBuild = false;
        bool hasBuildPreview = false;
        std::string buildPreviewText;
        std::string buildPreviewStatus = "No build yet.";
        AutoItPreprocessor::Compiler::CompilationUnit buildPreviewCompilation;
        std::future<RunTaskResult> runTask;
        std::shared_ptr<RunOutputBuffer> liveRunOutput;
        bool runInProgress = false;
    };

    inline constexpr ImVec4 kAccentColor = ImVec4(0.20f, 0.55f, 0.93f, 1.0f);
    inline constexpr ImVec4 kAccentSoftColor = ImVec4(0.17f, 0.27f, 0.38f, 1.0f);
    inline constexpr ImVec4 kPanelColor = ImVec4(0.09f, 0.11f, 0.15f, 1.0f);
    inline constexpr ImVec4 kPanelAltColor = ImVec4(0.11f, 0.13f, 0.18f, 1.0f);
    inline constexpr const char* kProjectExtension = ".torii";
    inline constexpr const char* kSourceExtension = ".aup";

    inline DocumentState& CurrentDocument(EditorState& state)
    {
        return state.documents.at(state.currentDocumentIndex);
    }

    inline const DocumentState& CurrentDocument(const EditorState& state)
    {
        return state.documents.at(state.currentDocumentIndex);
    }

    inline bool HasOpenDocument(const EditorState& state)
    {
        return !state.documents.empty() && state.currentDocumentIndex < state.documents.size();
    }
}

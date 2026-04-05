# Torii Labs Changelog

Torii Labs is the desktop editor for Torii projects and the AutoIt+ toolchain. This alpha release focuses on the core editing loop: project navigation, live source-to-generated previewing, theming, build and run tooling, and the first round of persistent settings.

## 0.0.1 Alpha

### Highlights

- Full Torii Labs desktop editor shell with project explorer, source editor, generated preview, symbols, output, and run panes.
- Live preview mapping from source lines to generated AutoIt, including include-expansion tracking.
- Read-only generated preview with dedicated block highlighting instead of text selection.
- Theme system with bundled presets: `Torii`, `Umi`, `Midnight`, `Forest`, and `Daylight`.
- JSON-based user, workspace, project, and shortcut settings.
- Configurable global shortcuts with an in-editor shortcuts preferences page.
- Project settings for choosing the main build target file.
- Source tree drag and drop, build-target context action, and persisted tree expansion state.
- ANSI-aware output rendering for build and run consoles.
- Asynchronous project tree loading, asynchronous symbol analysis, and non-blocking build polling.

### Build and Project Workflow

- Strict project layout with `code/`, `build/debug/`, and `build/release/`.
- Build target marker in the source tree and per-project main file selection.
- Release and debug build modes exposed in the editor.
- Console widgets that keep output readable and auto-scroll only when already at the bottom.

### Editing and Preview

- Pane-specific zoom behavior for source, preview, and output panes.
- Optional line numbers and global user-level whitespace preferences.
- Tabs are normalized to spaces, indentation backspace respects indentation stops, and save no longer forces an unwanted trailing newline.
- Better symbol navigation with centered jumps in both source and generated preview.

### Notes

- This is an alpha release. Behavior, settings layout, and workflow automation may still change while the toolchain stabilizes.

# Torii

`Torii` is a preprocessor and build tool for AutoIt code.
The project is organized as a CMake workspace with a root editor app and language-specific toolchains:

- `Torii.Labs`: desktop editor
- `toolchains/AutoIt+.Common`: shared data types
- `toolchains/AutoIt+.Lexer`: tokenizer library
- `toolchains/AutoIt+.Compiler`: include resolver, custom token rewriter, and emitter
- `toolchains/AutoIt+.Cli`: command-line frontend

## Build

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

If `deps/` is missing, CMake downloads the editor dependencies (`glfw` and `imgui`) automatically during configure via `FetchContent`.
If you want to allow only local or vendored dependencies, use `-DAUTOIT_FETCH_EDITOR_DEPS=OFF`.

The CLI binary will then be available at `bin/Release/AutoItPreprocessor/AutoItPreprocessor.exe`.
The editor will be available at `bin/Release/ToriiLabs/Torii Labs.exe`.

## Tokenize

```powershell
.\bin\Release\AutoItPreprocessor\AutoItPreprocessor.exe tokenize .\tests\data\root.au3
```

With include directories:

```powershell
.\bin\Release\AutoItPreprocessor\AutoItPreprocessor.exe tokenize .\tests\data\root.au3 --include-dir .\tests\data\includes
```

## Strip

The `strip` command loads all includes recursively, inserts each file at most once, and writes one large AutoIt file.
Without `--out`, it automatically creates `<filename>_stripped.au3`.

```powershell
.\bin\Release\AutoItPreprocessor\AutoItPreprocessor.exe strip .\tests\data\root.au3 --include-dir .\tests\data\includes
```

## Compile

The `compile` command expands includes recursively, replaces optional custom tokens, and writes valid AutoIt code again:

```powershell
.\bin\Release\AutoItPreprocessor\AutoItPreprocessor.exe compile .\tests\data\root.au3 --out .\build\compiled.au3 --include-dir .\tests\data\includes --custom .\tests\data\custom.tokens
```

Important: includes are now inserted globally only once. `#include-once` remains compatible, but is effectively redundant for this tool.
On Windows, `#include <file>` also loads these locations automatically:
- the AutoIt registry paths from `HKCU\Software\AutoIt v3\AutoIt\Include`
- the install include directory from `HKLM\SOFTWARE\WOW6432Node\AutoIt v3\AutoIt\InstallDir\Include`

## Editor

There is also a desktop app based on GLFW, OpenGL, and ImGui:

```powershell
.\bin\Release\ToriiLabs\Torii Labs.exe
```

The editor can:
- use native file dialogs for `Open`, `Save As`, and `Save Output As`
- edit AutoIt code with syntax highlighting in a proper editor widget
- perform undo/redo directly in the editor and through menu/shortcuts
- save and load Torii projects as `.torii`
- build the current buffer and write the result to `build/debug/<project>.au3` or `build/release/<project>.au3`
- run built projects directly from the editor
- configure semicolon-separated include and custom rule paths directly in the UI
- keep multiple files open as tabs

Shortcuts:
- `Ctrl+S`: save
- `Ctrl+Shift+S`: save as
- `Ctrl+B`: Build
- `F5`: Run
- `Ctrl+Z` / `Ctrl+Y`: Undo / Redo

## Custom Tokens

Custom tokens are defined through external rule files:

```text
token AppMessage
match=__APP_MESSAGE__
emit=$LOCAL_VALUE & " / " & $SHARED_VALUE
kinds=Word
end
```

Exact matches on individual tokens are currently supported. `emit` accepts `\n`, `\t`, `\r`, and `\\` as escapes.

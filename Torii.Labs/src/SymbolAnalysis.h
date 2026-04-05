#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace AutoItPlus::Editor
{
    struct EditorState;
    struct DocumentState;

    struct VariableSymbol
    {
        std::string name;
        int line = 1;
    };

    struct FunctionSymbol
    {
        std::string name;
        int line = 1;
        std::vector<VariableSymbol> parameters;
        std::vector<VariableSymbol> locals;
    };

    struct IncludeSymbol
    {
        std::string path;
        int line = 1;
        bool isSystem = false;
    };

    struct ExternalFileSymbols
    {
        std::filesystem::path sourcePath;
        std::vector<VariableSymbol> functions;
        std::vector<VariableSymbol> variables;
    };

    struct OutlineData
    {
        std::vector<FunctionSymbol> functions;
        std::vector<VariableSymbol> globals;
        std::vector<VariableSymbol> constants;
        std::vector<IncludeSymbol> includes;
        std::vector<ExternalFileSymbols> externals;
    };

    OutlineData AnalyzeOutline(const EditorState& state, const DocumentState& document);
}

#pragma once

#include "AutoItPreprocessor/Tokenizer/Token.h"
#include "AutoItPreprocessor/Common/SourceDocument.h"

#include <filesystem>
#include <string>
#include <vector>

namespace AutoItPreprocessor::Compiler
{
    struct CompilerOptions
    {
        std::vector<std::filesystem::path> includeDirectories;
        std::vector<std::filesystem::path> customRuleFiles;
    };

    struct LineMapping
    {
        std::filesystem::path sourcePath;
        std::size_t sourceLine = 0;
        std::size_t mergedSourceLine = 0;
        std::size_t generatedLineStart = 0;
        std::size_t generatedLineEnd = 0;
    };

    struct GeneratedIncludeExpansion
    {
        std::filesystem::path sourcePath;
        std::size_t sourceLine = 0;
        std::filesystem::path includedPath;
        std::size_t generatedLineStart = 0;
        std::size_t generatedLineEnd = 0;
        bool skipped = false;
    };

    struct CompilationUnit
    {
        std::filesystem::path rootPath;
        std::vector<std::filesystem::path> includedFiles;
        std::vector<Tokenizer::Token> tokens;
        std::string strippedCode;
        std::string generatedCode;
        std::vector<LineMapping> lineMappings;
        std::vector<GeneratedIncludeExpansion> includeExpansions;
    };

    class Compiler
    {
    public:
        [[nodiscard]] CompilationUnit Compile(const std::filesystem::path& inputFile, const CompilerOptions& options) const;
        [[nodiscard]] CompilationUnit Compile(const Common::SourceDocument& inputDocument, const CompilerOptions& options) const;
    };
}

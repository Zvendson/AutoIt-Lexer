#pragma once

#include "AutoItPreprocessor/Common/SourceDocument.h"

#include <filesystem>
#include <string>
#include <unordered_set>
#include <vector>

namespace AutoItPreprocessor::Compiler
{
    struct ResolvedLineOrigin
    {
        std::filesystem::path path;
        std::size_t line = 0;
    };

    struct IncludeExpansion
    {
        std::filesystem::path sourcePath;
        std::size_t sourceLine = 0;
        std::filesystem::path includedPath;
        std::size_t mergedLineStart = 0;
        std::size_t mergedLineEnd = 0;
        bool skipped = false;
    };

    struct IncludeResolveResult
    {
        Common::SourceDocument mergedDocument;
        std::vector<std::filesystem::path> includedFiles;
        std::vector<ResolvedLineOrigin> lineOrigins;
        std::vector<IncludeExpansion> includeExpansions;
    };

    class IncludeResolver
    {
    public:
        [[nodiscard]] IncludeResolveResult Resolve(const std::filesystem::path& rootPath, const std::vector<std::filesystem::path>& includeDirectories) const;
        [[nodiscard]] IncludeResolveResult Resolve(const Common::SourceDocument& rootDocument, const std::vector<std::filesystem::path>& includeDirectories) const;

    private:
        struct ParseResult
        {
            std::string mergedCode;
            std::vector<ResolvedLineOrigin> lineOrigins;
            std::vector<IncludeExpansion> includeExpansions;
            bool includeOnce = false;
        };

        [[nodiscard]] ParseResult ResolveDocumentText(
            const std::filesystem::path& filePath,
            const std::string& text,
            const std::vector<std::filesystem::path>& includeDirectories,
            std::unordered_set<std::filesystem::path>& seenFiles,
            std::vector<std::filesystem::path>& includedFiles) const;

        [[nodiscard]] ParseResult ResolveFile(
            const std::filesystem::path& filePath,
            const std::vector<std::filesystem::path>& includeDirectories,
            std::unordered_set<std::filesystem::path>& seenFiles,
            std::vector<std::filesystem::path>& includedFiles) const;
    };
}

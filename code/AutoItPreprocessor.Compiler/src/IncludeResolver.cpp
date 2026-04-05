#include "AutoItPreprocessor/Compiler/IncludeResolver.h"

#include <fstream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string_view>

#if defined(_WIN32)
#include <windows.h>
#include <winreg.h>
#endif

namespace
{
    std::string ReadFile(const std::filesystem::path& path)
    {
        std::ifstream input(path);
        if (!input.is_open())
            throw std::runtime_error("Could not open source file: " + path.string());

        return std::string((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    }

    std::string Trim(std::string value)
    {
        const auto begin = value.find_first_not_of(" \t\r\n");
        if (begin == std::string::npos)
            return "";

        const auto end = value.find_last_not_of(" \t\r\n");
        return value.substr(begin, end - begin + 1U);
    }

    bool StartsWithInclude(const std::string& line)
    {
        return line.starts_with("#include ");
    }

    std::vector<std::filesystem::path> LoadAutoItRegistryIncludeDirectories()
    {
        std::vector<std::filesystem::path> includeDirectories;

#if defined(_WIN32)
        constexpr wchar_t kSubKey[] = L"Software\\AutoIt v3\\AutoIt";
        constexpr wchar_t kValueName[] = L"Include";

        DWORD valueType = 0;
        DWORD valueSize = 0;
        const auto sizeResult = RegGetValueW(
            HKEY_CURRENT_USER,
            kSubKey,
            kValueName,
            RRF_RT_REG_SZ,
            &valueType,
            nullptr,
            &valueSize);

        if (sizeResult != ERROR_SUCCESS || valueSize == 0)
            return includeDirectories;

        std::wstring rawValue(valueSize / sizeof(wchar_t), L'\0');
        const auto readResult = RegGetValueW(
            HKEY_CURRENT_USER,
            kSubKey,
            kValueName,
            RRF_RT_REG_SZ,
            &valueType,
            rawValue.data(),
            &valueSize);

        if (readResult != ERROR_SUCCESS)
            return includeDirectories;

        const std::wstring_view registryValue(rawValue.c_str());

        std::wstring currentPart;
        for (const wchar_t character : registryValue)
        {
            if (character == L';')
            {
                if (!currentPart.empty())
                    includeDirectories.emplace_back(std::filesystem::path(currentPart).lexically_normal());
                currentPart.clear();
                continue;
            }

            currentPart += character;
        }

        if (!currentPart.empty())
            includeDirectories.emplace_back(std::filesystem::path(currentPart).lexically_normal());
#endif

        return includeDirectories;
    }

    std::optional<std::filesystem::path> LoadAutoItInstallIncludeDirectory()
    {
#if defined(_WIN32)
        constexpr const wchar_t* kSubKeys[] = {
            L"SOFTWARE\\WOW6432Node\\AutoIt v3\\AutoIt",
            L"SOFTWARE\\AutoIt v3\\AutoIt"
        };
        constexpr wchar_t kValueName[] = L"InstallDir";

        for (const auto* subKey : kSubKeys)
        {
            DWORD valueType = 0;
            DWORD valueSize = 0;
            const auto sizeResult = RegGetValueW(
                HKEY_LOCAL_MACHINE,
                subKey,
                kValueName,
                RRF_RT_REG_SZ,
                &valueType,
                nullptr,
                &valueSize);

            if (sizeResult != ERROR_SUCCESS || valueSize == 0)
                continue;

            std::wstring rawValue(valueSize / sizeof(wchar_t), L'\0');
            const auto readResult = RegGetValueW(
                HKEY_LOCAL_MACHINE,
                subKey,
                kValueName,
                RRF_RT_REG_SZ,
                &valueType,
                rawValue.data(),
                &valueSize);

            if (readResult != ERROR_SUCCESS)
                continue;

            const std::wstring_view installDir(rawValue.c_str());
            if (installDir.empty())
                continue;

            return (std::filesystem::path(installDir) / "Include").lexically_normal();
        }
#endif

        return std::nullopt;
    }

    std::vector<std::filesystem::path> MergeIncludeDirectories(const std::vector<std::filesystem::path>& includeDirectories)
    {
        std::vector<std::filesystem::path> mergedDirectories = includeDirectories;

        for (const auto& registryDirectory : LoadAutoItRegistryIncludeDirectories())
        {
            const auto canonicalCandidate = std::filesystem::path(registryDirectory).lexically_normal();
            const auto alreadyPresent = std::find_if(
                mergedDirectories.begin(),
                mergedDirectories.end(),
                [&](const std::filesystem::path& existing)
                {
                    return existing.lexically_normal() == canonicalCandidate;
                });

            if (alreadyPresent == mergedDirectories.end())
                mergedDirectories.push_back(registryDirectory);
        }

        if (const auto installIncludeDirectory = LoadAutoItInstallIncludeDirectory(); installIncludeDirectory.has_value())
        {
            const auto canonicalCandidate = installIncludeDirectory->lexically_normal();
            const auto alreadyPresent = std::find_if(
                mergedDirectories.begin(),
                mergedDirectories.end(),
                [&](const std::filesystem::path& existing)
                {
                    return existing.lexically_normal() == canonicalCandidate;
                });

            if (alreadyPresent == mergedDirectories.end())
                mergedDirectories.push_back(*installIncludeDirectory);
        }

        return mergedDirectories;
    }

    std::filesystem::path ResolveIncludePath(
        const std::string& line,
        const std::filesystem::path& currentFile,
        const std::vector<std::filesystem::path>& includeDirectories)
    {
        static const std::regex includeRegex(R"(^\s*#include\s+((<[^>]+>)|(\"[^\"]+\")))");
        std::smatch match;
        std::string payload;
        if (std::regex_search(line, match, includeRegex))
            payload = match[1].str();

        const bool localInclude = payload.size() >= 2U && payload.front() == '"' && payload.back() == '"';
        const bool globalInclude = payload.size() >= 2U && payload.front() == '<' && payload.back() == '>';

        if (!localInclude && !globalInclude)
            throw std::runtime_error("Invalid #include directive in " + currentFile.string() + ": " + line);

        const std::filesystem::path includeName = payload.substr(1, payload.size() - 2U);
        std::vector<std::filesystem::path> searchedPaths;

        if (localInclude)
        {
            const auto localPath = currentFile.parent_path() / includeName;
            searchedPaths.push_back(localPath);
            if (std::filesystem::is_regular_file(localPath))
                return std::filesystem::weakly_canonical(localPath);
        }

        for (const auto& includeDir : includeDirectories)
        {
            const auto candidate = includeDir / includeName;
            searchedPaths.push_back(candidate);
            if (std::filesystem::is_regular_file(candidate))
                return std::filesystem::weakly_canonical(candidate);
        }

        std::string message = "Could not resolve include " + includeName.string() + " from " + currentFile.string();
        if (!searchedPaths.empty())
        {
            message += "\nSearched paths:";
            for (const auto& searchedPath : searchedPaths)
            {
                message += "\n  - ";
                message += searchedPath.lexically_normal().string();
            }
        }

        throw std::runtime_error(message);
    }
}

namespace AutoItPreprocessor::Compiler
{
    IncludeResolveResult IncludeResolver::Resolve(const std::filesystem::path& rootPath, const std::vector<std::filesystem::path>& includeDirectories) const
    {
        return Resolve(Common::SourceDocument{
            .path = rootPath,
            .text = ReadFile(rootPath)
        }, includeDirectories);
    }

    IncludeResolveResult IncludeResolver::Resolve(const Common::SourceDocument& rootDocument, const std::vector<std::filesystem::path>& includeDirectories) const
    {
        const auto mergedIncludeDirectories = MergeIncludeDirectories(includeDirectories);
        std::unordered_set<std::filesystem::path> seenFiles;
        std::vector<std::filesystem::path> includedFiles;

        const auto canonicalRoot = std::filesystem::weakly_canonical(rootDocument.path);
        const auto parsed = ResolveDocumentText(canonicalRoot, rootDocument.text, mergedIncludeDirectories, seenFiles, includedFiles);

        return {
            .mergedDocument = Common::SourceDocument{canonicalRoot, parsed.mergedCode},
            .includedFiles = std::move(includedFiles),
            .lineOrigins = std::move(parsed.lineOrigins),
            .includeExpansions = std::move(parsed.includeExpansions)
        };
    }

    IncludeResolver::ParseResult IncludeResolver::ResolveDocumentText(
        const std::filesystem::path& filePath,
        const std::string& text,
        const std::vector<std::filesystem::path>& includeDirectories,
        std::unordered_set<std::filesystem::path>& seenFiles,
        std::vector<std::filesystem::path>& includedFiles) const
    {
        if (seenFiles.contains(filePath))
            return {};

        seenFiles.insert(filePath);
        includedFiles.push_back(filePath);

        std::stringstream stream(text);
        std::string mergedCode;
        std::string line;
        std::vector<ResolvedLineOrigin> lineOrigins;
        std::vector<IncludeExpansion> includeExpansions;
        bool includeOnce = false;
        std::size_t fileLine = 0;

        while (std::getline(stream, line))
        {
            ++fileLine;
            const auto trimmed = Trim(line);

            if (trimmed == "#include-once")
            {
                includeOnce = true;
                continue;
            }

            if (StartsWithInclude(trimmed))
            {
                const auto includePath = ResolveIncludePath(trimmed, filePath, includeDirectories);
                const std::size_t mergedLineStart = lineOrigins.size() + 1U;
                const auto nested = ResolveFile(includePath, includeDirectories, seenFiles, includedFiles);
                mergedCode += nested.mergedCode;
                lineOrigins.insert(lineOrigins.end(), nested.lineOrigins.begin(), nested.lineOrigins.end());
                includeExpansions.insert(includeExpansions.end(), nested.includeExpansions.begin(), nested.includeExpansions.end());
                if (!mergedCode.empty() && !mergedCode.ends_with('\n'))
                    mergedCode += '\n';

                const std::size_t mergedLineEnd = lineOrigins.size();
                includeExpansions.push_back(IncludeExpansion{
                    .sourcePath = filePath,
                    .sourceLine = fileLine,
                    .includedPath = includePath,
                    .mergedLineStart = mergedLineStart,
                    .mergedLineEnd = mergedLineEnd,
                    .skipped = mergedLineEnd < mergedLineStart
                });

                continue;
            }

            mergedCode += line;
            mergedCode += '\n';
            lineOrigins.push_back(ResolvedLineOrigin{
                .path = filePath,
                .line = fileLine
            });
        }

        return {
            .mergedCode = std::move(mergedCode),
            .lineOrigins = std::move(lineOrigins),
            .includeExpansions = std::move(includeExpansions),
            .includeOnce = includeOnce
        };
    }

    IncludeResolver::ParseResult IncludeResolver::ResolveFile(
        const std::filesystem::path& filePath,
        const std::vector<std::filesystem::path>& includeDirectories,
        std::unordered_set<std::filesystem::path>& seenFiles,
        std::vector<std::filesystem::path>& includedFiles) const
    {
        return ResolveDocumentText(filePath, ReadFile(filePath), includeDirectories, seenFiles, includedFiles);
    }
}

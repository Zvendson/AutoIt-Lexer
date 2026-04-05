#include "AutoItPreprocessor/Compiler/Compiler.h"

#include "AutoItPreprocessor/Compiler/CustomTokenRegistry.h"
#include "AutoItPreprocessor/Compiler/Emitter.h"
#include "AutoItPreprocessor/Compiler/IncludeResolver.h"
#include "AutoItPreprocessor/Tokenizer/Tokenizer.h"

namespace AutoItPreprocessor::Compiler
{
    namespace
    {
        std::vector<LineMapping> ResolveLineMappings(
            std::vector<LineMapping> mergedMappings,
            const std::vector<ResolvedLineOrigin>& lineOrigins,
            const std::filesystem::path& fallbackPath)
        {
            for (auto& mapping : mergedMappings)
            {
                if (mapping.sourceLine == 0 || mapping.sourceLine > lineOrigins.size())
                {
                    mapping.sourcePath = fallbackPath;
                    mapping.mergedSourceLine = mapping.sourceLine;
                    continue;
                }

                const auto& origin = lineOrigins[mapping.sourceLine - 1U];
                mapping.sourcePath = origin.path.empty() ? fallbackPath : origin.path;
                mapping.mergedSourceLine = mapping.sourceLine;
                mapping.sourceLine = origin.line;
            }

            return mergedMappings;
        }

        std::vector<GeneratedIncludeExpansion> ResolveIncludeExpansions(
            const std::vector<IncludeExpansion>& expansions,
            const std::vector<LineMapping>& lineMappings)
        {
            std::vector<GeneratedIncludeExpansion> resolved;
            resolved.reserve(expansions.size());

            for (const auto& expansion : expansions)
            {
                GeneratedIncludeExpansion generatedExpansion{
                    .sourcePath = expansion.sourcePath,
                    .sourceLine = expansion.sourceLine,
                    .includedPath = expansion.includedPath,
                    .skipped = expansion.skipped
                };

                if (!expansion.skipped)
                {
                    for (const auto& mapping : lineMappings)
                    {
                        if (mapping.generatedLineStart == 0 || mapping.generatedLineEnd == 0)
                            continue;
                        if (mapping.mergedSourceLine < expansion.mergedLineStart || mapping.mergedSourceLine > expansion.mergedLineEnd)
                            continue;

                        if (generatedExpansion.generatedLineStart == 0 || mapping.generatedLineStart < generatedExpansion.generatedLineStart)
                            generatedExpansion.generatedLineStart = mapping.generatedLineStart;
                        if (mapping.generatedLineEnd > generatedExpansion.generatedLineEnd)
                            generatedExpansion.generatedLineEnd = mapping.generatedLineEnd;
                    }

                    if (generatedExpansion.generatedLineStart == 0 || generatedExpansion.generatedLineEnd == 0)
                        generatedExpansion.skipped = true;
                }

                resolved.push_back(std::move(generatedExpansion));
            }

            return resolved;
        }
    }

    CompilationUnit Compiler::Compile(const std::filesystem::path& inputFile, const CompilerOptions& options) const
    {
        IncludeResolver includeResolver;
        const auto resolved = includeResolver.Resolve(inputFile, options.includeDirectories);

        Tokenizer::Tokenizer tokenizer(resolved.mergedDocument.text);
        auto tokens = tokenizer.TokenizeAll();

        CustomTokenRegistry registry;
        for (const auto& ruleFile : options.customRuleFiles)
            registry.LoadFromFile(ruleFile);

        for (auto& token : tokens)
        {
            const auto rule = registry.Match(token);
            if (rule.has_value())
                token.RebindAsCustom(rule->name, rule->emit);
        }

        Emitter emitter;
        auto emitResult = emitter.Emit(tokens);

        auto lineMappings = ResolveLineMappings(std::move(emitResult.lineMappings), resolved.lineOrigins, resolved.mergedDocument.path);
        auto includeExpansions = ResolveIncludeExpansions(resolved.includeExpansions, lineMappings);

        return {
            .rootPath = resolved.mergedDocument.path,
            .includedFiles = std::move(resolved.includedFiles),
            .tokens = std::move(tokens),
            .strippedCode = resolved.mergedDocument.text,
            .generatedCode = std::move(emitResult.code),
            .lineMappings = std::move(lineMappings),
            .includeExpansions = std::move(includeExpansions)
        };
    }

    CompilationUnit Compiler::Compile(const Common::SourceDocument& inputDocument, const CompilerOptions& options) const
    {
        IncludeResolver includeResolver;
        const auto resolved = includeResolver.Resolve(inputDocument, options.includeDirectories);

        Tokenizer::Tokenizer tokenizer(resolved.mergedDocument.text);
        auto tokens = tokenizer.TokenizeAll();

        CustomTokenRegistry registry;
        for (const auto& ruleFile : options.customRuleFiles)
            registry.LoadFromFile(ruleFile);

        for (auto& token : tokens)
        {
            const auto rule = registry.Match(token);
            if (rule.has_value())
                token.RebindAsCustom(rule->name, rule->emit);
        }

        Emitter emitter;
        auto emitResult = emitter.Emit(tokens);

        auto lineMappings = ResolveLineMappings(std::move(emitResult.lineMappings), resolved.lineOrigins, resolved.mergedDocument.path);
        auto includeExpansions = ResolveIncludeExpansions(resolved.includeExpansions, lineMappings);

        return {
            .rootPath = resolved.mergedDocument.path,
            .includedFiles = std::move(resolved.includedFiles),
            .tokens = std::move(tokens),
            .strippedCode = resolved.mergedDocument.text,
            .generatedCode = std::move(emitResult.code),
            .lineMappings = std::move(lineMappings),
            .includeExpansions = std::move(includeExpansions)
        };
    }
}

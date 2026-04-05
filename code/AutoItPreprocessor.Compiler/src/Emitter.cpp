#include "AutoItPreprocessor/Compiler/Emitter.h"

#include <algorithm>

namespace AutoItPreprocessor::Compiler
{
    namespace
    {
        std::size_t CountTouchedLines(const std::string& text)
        {
            if (text.empty())
                return 0;

            std::size_t touchedLines = 1;
            for (char character : text)
            {
                if (character == '\n')
                    ++touchedLines;
            }

            if (text.back() == '\n')
                --touchedLines;

            return std::max<std::size_t>(1, touchedLines);
        }

        void UpdateLineMapping(std::vector<LineMapping>& mappings, std::size_t sourceLine, std::size_t generatedLineStart, std::size_t generatedLineEnd)
        {
            if (sourceLine == 0 || generatedLineStart == 0 || generatedLineEnd == 0)
                return;

            if (mappings.size() <= sourceLine)
                mappings.resize(sourceLine + 1U);

            auto& mapping = mappings[sourceLine];
            if (mapping.sourceLine == 0)
            {
                mapping.sourceLine = sourceLine;
                mapping.generatedLineStart = generatedLineStart;
                mapping.generatedLineEnd = generatedLineEnd;
                return;
            }

            mapping.generatedLineStart = std::min(mapping.generatedLineStart, generatedLineStart);
            mapping.generatedLineEnd = std::max(mapping.generatedLineEnd, generatedLineEnd);
        }
    }

    EmitResult Emitter::Emit(const std::vector<Tokenizer::Token>& tokens) const
    {
        EmitResult result;
        std::size_t generatedLine = 1;

        for (const auto& token : tokens)
        {
            if (token.Is(Tokenizer::TokenKind::End))
                continue;

            const std::string& emittedText = token.Is(Tokenizer::TokenKind::Custom) ? token.GetReplacement() : token.GetContent();
            if (!emittedText.empty())
            {
                const std::size_t generatedLineStart = generatedLine;
                const std::size_t generatedLineEnd = generatedLineStart + CountTouchedLines(emittedText) - 1U;
                UpdateLineMapping(result.lineMappings, token.GetLine(), generatedLineStart, generatedLineEnd);
                if (token.GetLine() < result.lineMappings.size())
                    result.lineMappings[token.GetLine()].mergedSourceLine = token.GetLine();

                for (char character : emittedText)
                {
                    if (character == '\n')
                        ++generatedLine;
                }
            }

            if (token.Is(Tokenizer::TokenKind::Custom))
                result.code += token.GetReplacement();
            else
                result.code += token.GetContent();
        }

        return result;
    }
}

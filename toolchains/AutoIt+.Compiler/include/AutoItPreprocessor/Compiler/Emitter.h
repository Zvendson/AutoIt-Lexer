#pragma once

#include "AutoItPreprocessor/Compiler/Compiler.h"
#include "AutoItPreprocessor/Tokenizer/Token.h"

#include <string>
#include <vector>

namespace AutoItPreprocessor::Compiler
{
    struct EmitResult
    {
        std::string code;
        std::vector<LineMapping> lineMappings;
    };

    class Emitter
    {
    public:
        [[nodiscard]] EmitResult Emit(const std::vector<Tokenizer::Token>& tokens) const;
    };
}

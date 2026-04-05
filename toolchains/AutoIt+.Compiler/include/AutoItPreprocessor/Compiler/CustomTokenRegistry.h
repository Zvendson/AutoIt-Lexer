#pragma once

#include "AutoItPreprocessor/Tokenizer/Token.h"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace AutoItPreprocessor::Compiler
{
    struct CustomTokenRule
    {
        std::string name;
        std::string match;
        std::string emit;
        std::vector<Tokenizer::TokenKind> allowedKinds;
    };

    class CustomTokenRegistry
    {
    public:
        void LoadFromFile(const std::filesystem::path& path);
        [[nodiscard]] std::optional<CustomTokenRule> Match(const Tokenizer::Token& token) const;

    private:
        std::vector<CustomTokenRule> m_Rules;
    };
}

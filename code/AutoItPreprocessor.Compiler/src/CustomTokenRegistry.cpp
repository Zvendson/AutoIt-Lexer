#include "AutoItPreprocessor/Compiler/CustomTokenRegistry.h"

#include "AutoItPreprocessor/Tokenizer/Token.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace
{
    using AutoItPreprocessor::Tokenizer::TokenKind;

    std::string Trim(std::string value)
    {
        const auto begin = value.find_first_not_of(" \t\r\n");
        if (begin == std::string::npos)
            return "";

        const auto end = value.find_last_not_of(" \t\r\n");
        return value.substr(begin, end - begin + 1U);
    }

    std::string Unescape(std::string value)
    {
        std::string result;
        result.reserve(value.size());

        for (std::size_t index = 0; index < value.size(); ++index)
        {
            if (value[index] == '\\' && index + 1U < value.size())
            {
                const char next = value[index + 1U];
                switch (next)
                {
                    case 'n': result += '\n'; ++index; continue;
                    case 't': result += '\t'; ++index; continue;
                    case 'r': result += '\r'; ++index; continue;
                    case '\\': result += '\\'; ++index; continue;
                    default: break;
                }
            }

            result += value[index];
        }

        return result;
    }

    TokenKind ParseKind(const std::string& value)
    {
        const auto lowered = AutoItPreprocessor::Tokenizer::ToLowerCopy(value);
        if (lowered == "word") return TokenKind::Word;
        if (lowered == "keyword") return TokenKind::Keyword;
        if (lowered == "string") return TokenKind::String;
        if (lowered == "comment") return TokenKind::Comment;
        if (lowered == "command" || lowered == "autoitcommand") return TokenKind::AutoItCommand;
        if (lowered == "variable") return TokenKind::Variable;
        if (lowered == "macro") return TokenKind::Macro;
        if (lowered == "custom") return TokenKind::Custom;
        throw std::runtime_error("Unknown token kind in custom token file: " + value);
    }
}

namespace AutoItPreprocessor::Compiler
{
    void CustomTokenRegistry::LoadFromFile(const std::filesystem::path& path)
    {
        std::ifstream input(path);
        if (!input.is_open())
            throw std::runtime_error("Could not open custom token file: " + path.string());

        CustomTokenRule currentRule;
        bool inRule = false;
        std::string line;

        while (std::getline(input, line))
        {
            const auto trimmed = Trim(line);
            if (trimmed.empty() || trimmed.starts_with('#'))
                continue;

            if (trimmed.starts_with("token "))
            {
                if (inRule)
                    throw std::runtime_error("Nested token blocks are not allowed in " + path.string());

                inRule = true;
                currentRule = {};
                currentRule.name = Trim(trimmed.substr(6));
                continue;
            }

            if (trimmed == "end")
            {
                if (!inRule || currentRule.name.empty() || currentRule.match.empty())
                    throw std::runtime_error("Invalid token rule in " + path.string());

                if (currentRule.allowedKinds.empty())
                    currentRule.allowedKinds.push_back(TokenKind::Word);

                m_Rules.push_back(currentRule);
                inRule = false;
                currentRule = {};
                continue;
            }

            if (!inRule)
                throw std::runtime_error("Properties must be inside a token block in " + path.string());

            const auto separator = trimmed.find('=');
            if (separator == std::string::npos)
                throw std::runtime_error("Expected key=value in " + path.string());

            const auto key = Trim(trimmed.substr(0, separator));
            const auto value = Trim(trimmed.substr(separator + 1U));

            if (key == "match")
                currentRule.match = value;
            else if (key == "emit")
                currentRule.emit = Unescape(value);
            else if (key == "kinds")
            {
                currentRule.allowedKinds.clear();
                std::stringstream stream(value);
                std::string part;
                while (std::getline(stream, part, ','))
                    currentRule.allowedKinds.push_back(ParseKind(Trim(part)));
            }
            else
                throw std::runtime_error("Unknown custom token property: " + key);
        }

        if (inRule)
            throw std::runtime_error("Unterminated token block in " + path.string());
    }

    std::optional<CustomTokenRule> CustomTokenRegistry::Match(const Tokenizer::Token& token) const
    {
        for (const auto& rule : m_Rules)
        {
            if (rule.match != token.GetContent())
                continue;

            for (const auto allowedKind : rule.allowedKinds)
            {
                if (token.GetKind() == allowedKind)
                    return rule;
            }
        }

        return std::nullopt;
    }
}

#include "AutoItPreprocessor/Tokenizer/Token.h"

#include <algorithm>
#include <cctype>
#include <utility>

namespace AutoItPreprocessor::Tokenizer
{
    Token::Token(TokenKind kind, std::size_t line, std::size_t start, std::size_t end, std::string content)
        : m_Kind(kind), m_Line(line), m_Start(start), m_End(end), m_Content(std::move(content))
    {
    }

    void Token::RebindAsCustom(std::string customName, std::string replacement)
    {
        m_Kind = TokenKind::Custom;
        m_CustomName = std::move(customName);
        m_Replacement = std::move(replacement);
    }

    std::string ToLowerCopy(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return value;
    }

    std::string ToUpperCopy(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        return value;
    }

    const char* ToString(TokenKind kind) noexcept
    {
        switch (kind)
        {
            case TokenKind::Start: return "Start";
            case TokenKind::End: return "End";
            case TokenKind::Space: return "Space";
            case TokenKind::Tab: return "Tab";
            case TokenKind::LineFeed: return "LineFeed";
            case TokenKind::Decimals: return "Decimals";
            case TokenKind::Hex: return "Hex";
            case TokenKind::Keyword: return "Keyword";
            case TokenKind::Word: return "Word";
            case TokenKind::OpenedParen: return "OpenedParen";
            case TokenKind::ClosedParen: return "ClosedParen";
            case TokenKind::OpenedSquare: return "OpenedSquare";
            case TokenKind::ClosedSquare: return "ClosedSquare";
            case TokenKind::LessThan: return "LessThan";
            case TokenKind::GreaterThan: return "GreaterThan";
            case TokenKind::Equal: return "Equal";
            case TokenKind::Plus: return "Plus";
            case TokenKind::Minus: return "Minus";
            case TokenKind::Asterisk: return "Asterisk";
            case TokenKind::Slash: return "Slash";
            case TokenKind::Power: return "Power";
            case TokenKind::AutoItCommand: return "AutoItCommand";
            case TokenKind::Variable: return "Variable";
            case TokenKind::Object: return "Object";
            case TokenKind::Multiline: return "Multiline";
            case TokenKind::Comma: return "Comma";
            case TokenKind::Colon: return "Colon";
            case TokenKind::String: return "String";
            case TokenKind::Comment: return "Comment";
            case TokenKind::MultiComment: return "MultiComment";
            case TokenKind::Concatenate: return "Concatenate";
            case TokenKind::Questionmark: return "Questionmark";
            case TokenKind::Macro: return "Macro";
            case TokenKind::Custom: return "Custom";
            case TokenKind::Error: return "Error";
            default: return "Unknown";
        }
    }
}

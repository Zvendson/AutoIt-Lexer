#pragma once

#include <cstddef>
#include <string>

namespace AutoItPreprocessor::Tokenizer
{
    enum class TokenKind
    {
        Start,
        End,
        Space,
        Tab,
        LineFeed,
        Decimals,
        Hex,
        Keyword,
        Word,
        OpenedParen,
        ClosedParen,
        OpenedSquare,
        ClosedSquare,
        LessThan,
        GreaterThan,
        Equal,
        Plus,
        Minus,
        Asterisk,
        Slash,
        Power,
        AutoItCommand,
        Variable,
        Object,
        Multiline,
        Comma,
        Colon,
        String,
        Comment,
        MultiComment,
        Concatenate,
        Questionmark,
        Macro,
        Custom,
        Error,
    };

    class Token
    {
    public:
        Token() = default;
        Token(TokenKind kind, std::size_t line, std::size_t start, std::size_t end, std::string content);

        [[nodiscard]] TokenKind GetKind() const noexcept { return m_Kind; }
        [[nodiscard]] std::size_t GetLine() const noexcept { return m_Line; }
        [[nodiscard]] std::size_t GetStart() const noexcept { return m_Start; }
        [[nodiscard]] std::size_t GetEnd() const noexcept { return m_End; }
        [[nodiscard]] const std::string& GetContent() const noexcept { return m_Content; }
        [[nodiscard]] const std::string& GetCustomName() const noexcept { return m_CustomName; }
        [[nodiscard]] const std::string& GetReplacement() const noexcept { return m_Replacement; }
        [[nodiscard]] bool Is(TokenKind kind) const noexcept { return m_Kind == kind; }

        void RebindAsCustom(std::string customName, std::string replacement);

    private:
        TokenKind m_Kind = TokenKind::Start;
        std::size_t m_Line = 1;
        std::size_t m_Start = 0;
        std::size_t m_End = 0;
        std::string m_Content;
        std::string m_CustomName;
        std::string m_Replacement;
    };

    [[nodiscard]] std::string ToLowerCopy(std::string value);
    [[nodiscard]] std::string ToUpperCopy(std::string value);
    [[nodiscard]] const char* ToString(TokenKind kind) noexcept;
}

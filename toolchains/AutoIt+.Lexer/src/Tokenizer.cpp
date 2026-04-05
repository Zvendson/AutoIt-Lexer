#include "AutoItPreprocessor/Tokenizer/Tokenizer.h"

#include <algorithm>
#include <iterator>
#include <string>

namespace
{
    bool IsSpace(char c) noexcept
    {
        return c == ' ' || c == '\r';
    }

    bool IsDigit(char c) noexcept
    {
        return c >= '0' && c <= '9';
    }

    bool IsHexDigit(char c) noexcept
    {
        return IsDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    }

    bool IsIdentifierChar(char c) noexcept
    {
        return (c >= 'a' && c <= 'z')
            || (c >= 'A' && c <= 'Z')
            || (c >= '0' && c <= '9')
            || c == '_';
    }

    bool IsKeyword(const std::string& value)
    {
        using AutoItPreprocessor::Tokenizer::ToLowerCopy;
        const auto lowered = ToLowerCopy(value);

        static const char* keywords[] = {
            "false", "true", "continuecase", "continueloop", "default", "dim", "redim", "global",
            "local", "const", "byref", "do", "until", "enum", "exit", "exitloop", "for", "to", "in",
            "step", "next", "func", "return", "endfunc", "if", "then", "elseif", "else", "endif",
            "null", "select", "case", "endselect", "static", "switch", "endswitch", "while", "wend",
            "with", "endwith", "not", "and", "or"
        };

        return std::find(std::begin(keywords), std::end(keywords), lowered) != std::end(keywords);
    }
}

namespace AutoItPreprocessor::Tokenizer
{
    Tokenizer::Tokenizer(std::string sourceText)
        : m_SourceText(std::move(sourceText))
    {
        m_Begin = m_SourceText.c_str();
        m_Cursor = m_Begin;
        m_End = m_Begin + m_SourceText.size();
    }

    Token Tokenizer::Peek()
    {
        const auto current = m_Current;
        const auto cursor = m_Cursor;
        const auto line = m_Line;

        const auto peeked = Next();

        m_Current = current;
        m_Cursor = cursor;
        m_Line = line;

        return peeked;
    }

    Token Tokenizer::Next()
    {
        const char curr = GetCurrent();
        if (curr == '\0')
        {
            const auto offset = static_cast<std::size_t>(m_Cursor - m_Begin);
            m_Current = Token(TokenKind::End, m_Line, offset, offset, "");
            return m_Current;
        }

        if (IsSpace(curr))
        {
            m_Current = MakeSpace();
            return m_Current;
        }

        if (curr == '\t')
        {
            m_Current = MakeTab();
            return m_Current;
        }

        const auto next = PeekNextChar();
        if (curr == '0' && (next == 'x' || next == 'X'))
        {
            m_Current = MakeHex();
            return m_Current;
        }

        if (IsDigit(curr))
        {
            m_Current = MakeDecimals();
            return m_Current;
        }

        if (curr == '_' && (IsSpace(next) || next == '\n'))
        {
            m_Current = MakeMultiline();
            return m_Current;
        }

        if (IsIdentifierChar(curr))
        {
            m_Current = MakeIdentifier();
            return m_Current;
        }

        switch (curr)
        {
            case '\'':
                m_Current = MakeString('\'');
                break;
            case '"':
                m_Current = MakeString('"');
                break;
            case '@':
                m_Current = MakeMacro();
                break;
            case ';':
                m_Current = MakeComment();
                break;
            case '#':
                m_Current = PeekLine().starts_with("#cs") || PeekLine().starts_with("#comment-start") ? MakeMultiComment() : MakeCommand();
                break;
            case '$':
                m_Current = MakeVariable();
                break;
            case '.':
                m_Current = MakeObject();
                break;
            case '(':
                m_Current = MakeSingle(TokenKind::OpenedParen);
                break;
            case ')':
                m_Current = MakeSingle(TokenKind::ClosedParen);
                break;
            case '[':
                m_Current = MakeSingle(TokenKind::OpenedSquare);
                break;
            case ']':
                m_Current = MakeSingle(TokenKind::ClosedSquare);
                break;
            case '<':
                m_Current = MakeSingle(TokenKind::LessThan);
                break;
            case '>':
                m_Current = MakeSingle(TokenKind::GreaterThan);
                break;
            case '=':
                m_Current = MakeSingle(TokenKind::Equal);
                break;
            case '+':
                m_Current = MakeSingle(TokenKind::Plus);
                break;
            case '-':
                m_Current = MakeSingle(TokenKind::Minus);
                break;
            case '*':
                m_Current = MakeSingle(TokenKind::Asterisk);
                break;
            case '/':
                m_Current = MakeSingle(TokenKind::Slash);
                break;
            case '^':
                m_Current = MakeSingle(TokenKind::Power);
                break;
            case ',':
                m_Current = MakeSingle(TokenKind::Comma);
                break;
            case ':':
                m_Current = MakeSingle(TokenKind::Colon);
                break;
            case '&':
                m_Current = MakeSingle(TokenKind::Concatenate);
                break;
            case '?':
                m_Current = MakeSingle(TokenKind::Questionmark);
                break;
            case '\n':
                m_Current = MakeSingle(TokenKind::LineFeed);
                m_Line++;
                break;
            default:
                m_Current = MakeError();
                break;
        }

        return m_Current;
    }

    std::vector<Token> Tokenizer::TokenizeAll()
    {
        std::vector<Token> tokens;

        while (true)
        {
            auto token = Next();
            tokens.push_back(token);
            if (token.Is(TokenKind::End) || token.Is(TokenKind::Error))
                break;
        }

        return tokens;
    }

    void Tokenizer::Reset() noexcept
    {
        m_Cursor = m_Begin;
        m_Line = 1;
        m_Current = {};
    }

    Token Tokenizer::MakeSpace()
    {
        const char* start = m_Cursor;
        GetNext();

        while (IsSpace(GetCurrent()))
            GetNext();

        const auto startOffset = static_cast<std::size_t>(start - m_Begin);
        const auto endOffset = static_cast<std::size_t>(m_Cursor - m_Begin);
        return Token(TokenKind::Space, m_Line, startOffset, endOffset, std::string(start, m_Cursor));
    }

    Token Tokenizer::MakeTab()
    {
        const auto startOffset = static_cast<std::size_t>(m_Cursor - m_Begin);
        const char content = GetNext();
        return Token(TokenKind::Tab, m_Line, startOffset, startOffset + 1U, std::string(1, content));
    }

    Token Tokenizer::MakeIdentifier()
    {
        const char* start = m_Cursor;
        GetNext();
        while (IsIdentifierChar(GetCurrent()))
            GetNext();

        const auto startOffset = static_cast<std::size_t>(start - m_Begin);
        const auto endOffset = static_cast<std::size_t>(m_Cursor - m_Begin);
        std::string content(start, m_Cursor);
        const auto kind = IsKeyword(content) ? TokenKind::Keyword : TokenKind::Word;
        return Token(kind, m_Line, startOffset, endOffset, std::move(content));
    }

    Token Tokenizer::MakeMultiline()
    {
        const auto startOffset = static_cast<std::size_t>(m_Cursor - m_Begin);
        const char* start = m_Cursor;
        GetNext();
        const auto endOffset = static_cast<std::size_t>(m_Cursor - m_Begin);
        return Token(TokenKind::Multiline, m_Line, startOffset, endOffset, std::string(start, m_Cursor));
    }

    Token Tokenizer::MakeDecimals()
    {
        const char* start = m_Cursor;
        GetNext();
        while (IsDigit(GetCurrent()))
            GetNext();

        const auto startOffset = static_cast<std::size_t>(start - m_Begin);
        const auto endOffset = static_cast<std::size_t>(m_Cursor - m_Begin);
        return Token(TokenKind::Decimals, m_Line, startOffset, endOffset, std::string(start, m_Cursor));
    }

    Token Tokenizer::MakeHex()
    {
        const char* start = m_Cursor;
        GetNext();
        if (GetCurrent() == 'x' || GetCurrent() == 'X')
        {
            GetNext();
            while (IsHexDigit(GetCurrent()))
                GetNext();
        }

        const auto startOffset = static_cast<std::size_t>(start - m_Begin);
        const auto endOffset = static_cast<std::size_t>(m_Cursor - m_Begin);
        return Token(TokenKind::Hex, m_Line, startOffset, endOffset, std::string(start, m_Cursor));
    }

    Token Tokenizer::MakeCommand()
    {
        const char* start = m_Cursor;
        GetNext();
        while (GetCurrent() != '\0' && GetCurrent() != '\n')
            GetNext();

        const auto startOffset = static_cast<std::size_t>(start - m_Begin);
        const auto endOffset = static_cast<std::size_t>(m_Cursor - m_Begin);
        return Token(TokenKind::AutoItCommand, m_Line, startOffset, endOffset, std::string(start, m_Cursor));
    }

    Token Tokenizer::MakeVariable()
    {
        const char* start = m_Cursor;
        GetNext();
        while (IsIdentifierChar(GetCurrent()))
            GetNext();

        const auto startOffset = static_cast<std::size_t>(start - m_Begin);
        const auto endOffset = static_cast<std::size_t>(m_Cursor - m_Begin);
        return Token(TokenKind::Variable, m_Line, startOffset, endOffset, std::string(start, m_Cursor));
    }

    Token Tokenizer::MakeObject()
    {
        const char* start = m_Cursor;
        GetNext();
        while (IsIdentifierChar(GetCurrent()))
            GetNext();

        const auto startOffset = static_cast<std::size_t>(start - m_Begin);
        const auto endOffset = static_cast<std::size_t>(m_Cursor - m_Begin);
        return Token(TokenKind::Object, m_Line, startOffset, endOffset, std::string(start, m_Cursor));
    }

    Token Tokenizer::MakeComment()
    {
        const char* start = m_Cursor;
        GetNext();
        while (GetCurrent() != '\0' && GetCurrent() != '\n')
            GetNext();

        const auto startOffset = static_cast<std::size_t>(start - m_Begin);
        const auto endOffset = static_cast<std::size_t>(m_Cursor - m_Begin);
        return Token(TokenKind::Comment, m_Line, startOffset, endOffset, std::string(start, m_Cursor));
    }

    Token Tokenizer::MakeMultiComment()
    {
        const auto startLine = m_Line;
        const char* start = m_Cursor;

        while (GetCurrent() != '\0')
        {
            if (GetCurrent() == '\n')
            {
                m_Line++;
            }
            else if (GetCurrent() == '#')
            {
                const auto line = PeekLine();
                if (line.starts_with("#ce") || line.starts_with("#comment-end"))
                {
                    NextLine();
                    break;
                }
            }

            GetNext();
        }

        const auto startOffset = static_cast<std::size_t>(start - m_Begin);
        const auto endOffset = static_cast<std::size_t>(m_Cursor - m_Begin);
        return GetCurrent() == '\0'
            ? Token(TokenKind::Error, startLine, startOffset, endOffset, std::string(start, m_Cursor))
            : Token(TokenKind::MultiComment, startLine, startOffset, endOffset, std::string(start, m_Cursor));
    }

    Token Tokenizer::MakeString(char endSymbol)
    {
        const char* start = m_Cursor;
        GetNext();

        while (GetCurrent() != '\0')
        {
            const char current = GetNext();
            if (current == '\n')
                m_Line++;

            if (current == endSymbol)
            {
                const auto startOffset = static_cast<std::size_t>(start - m_Begin);
                const auto endOffset = static_cast<std::size_t>(m_Cursor - m_Begin);
                return Token(TokenKind::String, m_Line, startOffset, endOffset, std::string(start, m_Cursor));
            }
        }

        const auto startOffset = static_cast<std::size_t>(start - m_Begin);
        const auto endOffset = static_cast<std::size_t>(m_Cursor - m_Begin);
        return Token(TokenKind::Error, m_Line, startOffset, endOffset, std::string(start, m_Cursor));
    }

    Token Tokenizer::MakeMacro()
    {
        const char* start = m_Cursor;
        GetNext();
        while (IsIdentifierChar(GetCurrent()))
            GetNext();

        const auto startOffset = static_cast<std::size_t>(start - m_Begin);
        const auto endOffset = static_cast<std::size_t>(m_Cursor - m_Begin);
        return Token(TokenKind::Macro, m_Line, startOffset, endOffset, std::string(start, m_Cursor));
    }

    Token Tokenizer::MakeSingle(TokenKind kind)
    {
        const auto startOffset = static_cast<std::size_t>(m_Cursor - m_Begin);
        const char current = GetNext();
        return Token(kind, m_Line, startOffset, startOffset + 1U, std::string(1, current));
    }

    Token Tokenizer::MakeError()
    {
        return MakeSingle(TokenKind::Error);
    }

    std::string Tokenizer::PeekLine() const
    {
        const char* end = m_Cursor;
        while (*end != '\0' && *end != '\n')
            end++;

        return std::string(m_Cursor, end);
    }

    std::string Tokenizer::NextLine()
    {
        const char* start = m_Cursor;
        while (GetCurrent() != '\0' && GetCurrent() != '\n')
            GetNext();

        return std::string(start, m_Cursor);
    }

    char Tokenizer::GetCurrent() const noexcept
    {
        return m_Cursor >= m_End ? '\0' : *m_Cursor;
    }

    char Tokenizer::GetNext() noexcept
    {
        if (m_Cursor >= m_End)
            return '\0';

        return *m_Cursor++;
    }

    char Tokenizer::PeekNextChar() const noexcept
    {
        return (m_Cursor + 1) >= m_End ? '\0' : *(m_Cursor + 1);
    }
}

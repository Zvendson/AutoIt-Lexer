#include <algorithm>
#include <string>
#include <vector>
#include <iomanip>
#include <iostream>
#include <fstream>

#include "au3Tokenizer.h"



namespace
{
    bool is_space(char c) noexcept
    {
        switch (c)
        {
            case ' ':
            case '\t':
            case '\r':
                return true;
            default:
                return false;
        }
    }

    bool is_digit(char c) noexcept
    {
        switch (c)
        {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                return true;
            default:
                return false;
        }
    }

    bool is_hex_digit(char c) noexcept
    {
        switch (c)
        {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            case 'a':
            case 'b':
            case 'c':
            case 'd':
            case 'e':
            case 'f':
            case 'A':
            case 'B':
            case 'C':
            case 'D':
            case 'E':
            case 'F':
                return true;
            default:
                return false;
        }
    }

    bool is_end_char(char c) noexcept
    {
       return c == '\0';
    }

    bool is_identifier_char(char c) noexcept
    {
        switch (c) {
            case 'a':
            case 'b':
            case 'c':
            case 'd':
            case 'e':
            case 'f':
            case 'g':
            case 'h':
            case 'i':
            case 'j':
            case 'k':
            case 'l':
            case 'm':
            case 'n':
            case 'o':
            case 'p':
            case 'q':
            case 'r':
            case 's':
            case 't':
            case 'u':
            case 'v':
            case 'w':
            case 'x':
            case 'y':
            case 'z':
            case 'A':
            case 'B':
            case 'C':
            case 'D':
            case 'E':
            case 'F':
            case 'G':
            case 'H':
            case 'I':
            case 'J':
            case 'K':
            case 'L':
            case 'M':
            case 'N':
            case 'O':
            case 'P':
            case 'Q':
            case 'R':
            case 'S':
            case 'T':
            case 'U':
            case 'V':
            case 'W':
            case 'X':
            case 'Y':
            case 'Z':
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            case '_':
                return true;
            default:
                return false;
        }
    }


    std::string lower(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
        return s;
    }


    bool is_au3_keyword(std::string& str)
    {
        auto cmp = lower(str);

        if (cmp == "false")
            return true;
        else if (cmp == "true")
            return true;
        else if (cmp == "continuecase")
            return true;
        else if (cmp == "continueloop")
            return true;
        else if (cmp == "default")
            return true;
        else if (cmp == "dim")
            return true;
        else if (cmp == "redim")
            return true;
        else if (cmp == "global")
            return true;
        else if (cmp == "local")
            return true;
        else if (cmp == "const")
            return true;
        else if (cmp == "byref")
            return true;
        else if (cmp == "do")
            return true;
        else if (cmp == "until")
            return true;
        else if (cmp == "enum")
            return true;
        else if (cmp == "exit")
            return true;
        else if (cmp == "exitloop")
            return true;
        else if (cmp == "for")
            return true;
        else if (cmp == "to")
            return true;
        else if (cmp == "in")
            return true;
        else if (cmp == "step")
            return true;
        else if (cmp == "next")
            return true;
        else if (cmp == "func")
            return true;
        else if (cmp == "return")
            return true;
        else if (cmp == "endfunc")
            return true;
        else if (cmp == "if")
            return true;
        else if (cmp == "then")
            return true;
        else if (cmp == "elseif")
            return true;
        else if (cmp == "else")
            return true;
        else if (cmp == "endif")
            return true;
        else if (cmp == "null")
            return true;
        else if (cmp == "select")
            return true;
        else if (cmp == "case")
            return true;
        else if (cmp == "endselect")
            return true;
        else if (cmp == "static")
            return true;
        else if (cmp == "switch")
            return true;
        else if (cmp == "endswitch")
            return true;
        else if (cmp == "while")
            return true;
        else if (cmp == "wend")
            return true;
        else if (cmp == "with")
            return true;
        else if (cmp == "endwith")
            return true;

        return false;
    }

}


namespace Au3
{
    Tokenizer::Tokenizer(const char* begin) noexcept
        : m_Begin{begin}, m_Cursor{begin}
    {
        while (*begin != '\0')
            begin++;
        m_End = begin;
    }

    Token Tokenizer::Next() noexcept
    {
        char curr = GetCurrent();

        if (is_end_char(curr))
        {
            m_Current = Token(Kind::End, m_Line, m_Begin, m_Cursor, 1);
            return m_Current;
        }

        if (is_space(curr))
        {
            m_Current = MakeSpace();
            return m_Current;
        }


        auto next = PeekNext();
        // Hex
        if (curr == '0' && next == 'x')
        {
            m_Current = MakeHex();
            return m_Current;
        }

        // Number
        if (is_digit(curr))
        {
            m_Current = MakeDecimals();
            return m_Current;
        }

        // Continue Statement
        if (curr == '_')
        {
            if (is_space(next) || next == '\n')
            {
                m_Current = MakeMultiline();
                return m_Current;
            }
        }

        // Identifier
        if (is_identifier_char(curr))
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
                {
                    auto line = PeekLine();
                    if (line.starts_with("#cs") || line.starts_with("#comment-start"))
                        m_Current = MakeMultiComment();
                    else 
                        m_Current = MakeCommand();
                }
                break;

            case '$':
                m_Current = MakeVariable();
                break;

            case '.':
                m_Current = MakeObject();
                break;

            case '(':
                m_Current = MakeSingle(Kind::OpenedParen);
                break;

            case ')':
                m_Current = MakeSingle(Kind::ClosedParen);
                break;

            case '[':
                m_Current = MakeSingle(Kind::OpenedSquare);
                break;

            case ']':
                m_Current = MakeSingle(Kind::ClosedSquare);
                break;

            case '<':
                m_Current = MakeSingle(Kind::LessThan);
                break;

            case '>':
                m_Current = MakeSingle(Kind::GreaterThan);
                break;

            case '=':
                m_Current = MakeSingle(Kind::Equal);
                break;

            case '+':
                m_Current = MakeSingle(Kind::Plus);
                break;

            case '-':
                m_Current = MakeSingle(Kind::Minus);
                break;

            case '*':
                m_Current = MakeSingle(Kind::Asterisk);
                break;

            case '/':
                m_Current = MakeSingle(Kind::Slash);
                break;

            case '^':
                m_Current = MakeSingle(Kind::Power);
                break;

            case ',':
                m_Current = MakeSingle(Kind::Comma);
                break;

            case ':':
                m_Current = MakeSingle(Kind::Colon);
                break;

            case '&':
                m_Current = MakeSingle(Kind::Concatenate);
                break;

            case '?':
                m_Current = MakeSingle(Kind::Questionmark);
                break;

            case '\n':
                m_Current = MakeSingle(Kind::LineFeed);
                m_Line++;
                break;

            default:
                m_Current = MakeError();
                break;
        }

        return m_Current;
    }

    Token Tokenizer::Next(Kind find) noexcept
    {
        while (true)
        {
            auto token = Next();
            if (token.IsOneOf(find, Kind::Error, Kind::End))
                return token;
        }
    }

    Token Tokenizer::MakeSingle(Kind kind) noexcept
    {
        return Token(kind, m_Line, m_Begin, m_Cursor++, 1);
    }

    Token Tokenizer::MakeError() noexcept
    {
        return Token(Kind::Error, m_Line, m_Begin, m_Cursor++, 1);
    }

    std::string Tokenizer::PeekLine() noexcept
    {
        const char* start = m_Cursor;
        const char* end = m_Cursor;

        while (*end != '\0')
        {
            if (*end == '\n')
                break;
            
            end++;
        }

        return std::string(start, end);
    }

    std::string Tokenizer::NextLine() noexcept
    {
        const char* start = m_Cursor;
        GetNext();

        while (GetCurrent() != '\0')
        {
            if (GetCurrent() == '\n')
            {
                break;
            }

            GetNext();
        }

        return std::string(start, m_Cursor);
    }

    Token Tokenizer::MakeSpace() noexcept
    {
        const char* start = m_Cursor;
        GetNext();

        while (is_space(GetCurrent())) GetNext();

        return Token(Kind::Space, m_Line, m_Begin, start, std::distance(start, m_Cursor));
    }

    Token Tokenizer::MakeIdentifier() noexcept
    {
        const char* start = m_Cursor;
        GetNext();

        while (is_identifier_char(GetCurrent())) GetNext();

        auto length = std::distance(start, m_Cursor);
        std::string content = {start, m_Cursor};

        Kind kind = Kind::Keyword;
        if (!is_au3_keyword(content))
            kind = Kind::Word;

        return Token(kind, m_Line, m_Begin, start, length);
    }

    Token Tokenizer::MakeMultiline() noexcept
    {
        const char* start = m_Cursor;
        GetNext();

        return Token(Kind::Multiline, m_Line, m_Begin, start, m_Cursor);

    }

    Token Tokenizer::MakeDecimals() noexcept
    {
        const char* start = m_Cursor;
        GetNext();

        while (is_digit(GetCurrent())) GetNext();

        return Token(Kind::Decimals, m_Line, m_Begin, start, m_Cursor);
    }

    Token Tokenizer::MakeHex() noexcept
    {
        const char* start = m_Cursor;
        GetNext();

        if (GetCurrent() == 'x' || GetCurrent() == 'X') {
            GetNext();
            while (is_hex_digit(GetCurrent())) GetNext();
            return Token(Kind::Hex, m_Line, m_Begin, start, m_Cursor);
        }
        else
        {
            while (is_digit(GetCurrent())) GetNext();
            return Token(Kind::Decimals, m_Line, m_Begin, start, m_Cursor);
        }
    }

    Token Tokenizer::MakeCommand() noexcept
    {
        const char* start = m_Cursor;
        GetNext();

        while (GetCurrent() != '\0')
        {
            if (GetCurrent() == '\n')
            {
                return Token(Kind::AutoItCommand, m_Line, m_Begin, start, m_Cursor);
            }
            GetNext();
        }

        return Token(Kind::Error, m_Line, m_Begin, m_Cursor, 1);
    }

    Token Tokenizer::MakeVariable() noexcept
    {
        const char* start = m_Cursor;
        GetNext();

        while (is_identifier_char(GetCurrent())) GetNext();

        return Token(Kind::Variable, m_Line, m_Begin, start, m_Cursor);
    }

    Token Tokenizer::MakeObject() noexcept
    {
        const char* start = m_Cursor;
        GetNext();

        while (is_identifier_char(GetCurrent())) GetNext();

        return Token(Kind::Object, m_Line, m_Begin, start, m_Cursor);
    }

    Token Tokenizer::MakeMacro() noexcept
    {
        const char* start = m_Cursor;
        GetNext();

        while (is_identifier_char(GetCurrent())) GetNext();

        return Token(Kind::Macro, m_Line, m_Begin, start, m_Cursor);
    }


    Token Tokenizer::MakeComment() noexcept
    {
        const char* start = m_Cursor;
        GetNext();

        while (GetCurrent() != '\0')
        {
            if (GetCurrent() == '\n')
            {
                return Token(Kind::Comment, m_Line, m_Begin, start, m_Cursor);
            }
            GetNext();
        }

        return Token(Kind::Error, m_Line, m_Begin, m_Cursor, 1);
    }

    Token Tokenizer::MakeMultiComment() noexcept
    {
        const char* start  = m_Cursor;
        size_t      linenr = m_Line;

        while (GetCurrent() != '\0')
        {                
            switch (GetCurrent())
            {
                case '\n':
                    m_Line++;
                    break;

                case '#':
                    auto line = PeekLine();
                    if (line.starts_with("#ce") || line.starts_with("#comment-end"))
                    {
                        NextLine();
                        return Token(Kind::MultiComment, linenr, m_Begin, start, m_Cursor);
                    }
            }

            GetNext();
        }

        return Token(Kind::Error, linenr, m_Begin, start, m_Cursor);
    }

    Token Tokenizer::MakeString(char endsymbol) noexcept
    {
        const char* start = m_Cursor;
        GetNext();

        while (GetCurrent() != '\0')
        {
            if (GetNext() == endsymbol)
            {
                return Token(Kind::String, m_Line, m_Begin, start, m_Cursor);
            }

        }

        return Token(Kind::Error, m_Line, m_Begin, m_Cursor, 1);
    }

}

















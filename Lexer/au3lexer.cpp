#include <algorithm>
#include <string>
#include <vector>
#include <iomanip>
#include <iostream>
#include <fstream>

#include "au3lexer.h"


Token Lexer::Next() noexcept 
{
    char curr = GetCurrent();
    auto next = PeekNext();



    if (curr == '\0')
    {
        m_current = Token(Kind::End, m_line, m_begin, m_cursor, 1);
        return m_current;
    }

    // Space
    if (is_space(curr))
    {
        m_current = MakeSpace();
        return m_current;
    }

    // Hex
    if (curr == '0' && next == 'x')
    {
        m_current = MakeHex();
        return m_current;
    }

    // Number
    if (is_digit(curr))
    {
        m_current = MakeDecimals();
        return m_current;
    }

    // Continue Statement
    if (curr == '_')
    {        
        if (is_space(next) || next == '\n')
        {
            m_current = MakeMultiline();
            return m_current;
        }
    }

    // Identifier
    if (is_identifier_char(curr))
    {
        m_current = MakeIdentifier();
        return m_current;
    }

    switch (curr) 
    {
        case '\'':
            m_current = MakeString('\'');
            break;
        case '"':
            m_current = MakeString('"');
            break;
        case '@':
            m_current = MakeMacro();
            break;
        case ';':
            m_current = MakeComment();
            break;
        case '#':
            m_current = MakeCommand();
            break;
        case '$':
            m_current = MakeVariable();
            break;
        case '.':
            m_current = MakeObject();
            break;
        case '(':
            m_current = MakeSingle(Kind::OpenedParen);
            break;
        case ')':
            m_current = MakeSingle(Kind::ClosedParen);
            break;
        case '[':
            m_current = MakeSingle(Kind::OpenedSquare);
            break;
        case ']':
            m_current = MakeSingle(Kind::ClosedSquare);
            break;
        case '<':
            m_current = MakeSingle(Kind::LessThan);
            break;
        case '>':
            m_current = MakeSingle(Kind::GreaterThan);
            break;
        case '=':
            m_current = MakeSingle(Kind::Equal);
            break;
        case '+':
            m_current = MakeSingle(Kind::Plus);
            break;
        case '-':
            m_current = MakeSingle(Kind::Minus);
            break;
        case '*':
            m_current = MakeSingle(Kind::Asterisk);
            break;
        case '/':
            m_current = MakeSingle(Kind::Slash);
            break;
        case '^':
            m_current = MakeSingle(Kind::Power);
            break;
        case ',':
            m_current = MakeSingle(Kind::Comma);
            break;
        case ':':
            m_current = MakeSingle(Kind::Colon);
            break;
        case '&':
            m_current = MakeSingle(Kind::Concatenate);
            break;
        case '?':
            m_current = MakeSingle(Kind::Questionmark);
            break;
        case '\n':
            m_current = MakeSingle(Kind::LineFeed);
            m_line++;
            break;
        default:
            m_current = MakeError();
            break;
    }

    return m_current;
}

Token Lexer::Next(Kind find) noexcept
{
    while (true)
    {
        auto token = Next();
        if (token.IsOneOf(find, Kind::Error, Kind::End))
            return token;
    }
}

Token Lexer::MakeSingle(Kind kind) noexcept
{
    return Token(kind, m_line, m_begin, m_cursor++, 1);
}

Token Lexer::MakeError() noexcept
{
    return Token(Kind::Error, m_line, m_begin, m_cursor++, 1);
}

Token Lexer::MakeSpace() noexcept
{
    const char* start = m_cursor;
    GetNext();

    while (is_space(GetCurrent())) GetNext();

    return Token(Kind::Space, m_line, m_begin, start, std::distance(start, m_cursor));
}

Token Lexer::MakeIdentifier() noexcept
{
    const char* start = m_cursor;
    GetNext();

    while (is_identifier_char(GetCurrent())) GetNext();

    auto length = std::distance(start, m_cursor);
    std::string content = lower({ start, m_cursor });

    Kind kind = Kind::Word;

    if (content == lower(GetKindName(Kind::False)))
        kind = Kind::False;

    else if (content == lower(GetKindName(Kind::True)))
        kind = Kind::True;

    else if (content == lower(GetKindName(Kind::ContinueCase)))
        kind = Kind::ContinueCase;

    else if (content == lower(GetKindName(Kind::ContinueLoop)))
        kind = Kind::ContinueLoop;

    else if (content == lower(GetKindName(Kind::Default)))
        kind = Kind::Default;

    else if (content == lower(GetKindName(Kind::Dim)))
        kind = Kind::Dim;

    else if (content == lower(GetKindName(Kind::ReDim)))
        kind = Kind::ReDim;

    else if (content == lower(GetKindName(Kind::Global)))
        kind = Kind::Global;

    else if (content == lower(GetKindName(Kind::Local)))
        kind = Kind::Local;

    else if (content == lower(GetKindName(Kind::Const)))
        kind = Kind::Const;

    else if (content == lower(GetKindName(Kind::ByRef)))
        kind = Kind::ByRef;

    else if (content == lower(GetKindName(Kind::Do)))
        kind = Kind::Do;

    else if (content == lower(GetKindName(Kind::Until)))
        kind = Kind::Until;

    else if (content == lower(GetKindName(Kind::Enum)))
        kind = Kind::Enum;

    else if (content == lower(GetKindName(Kind::Exit)))
        kind = Kind::Exit;

    else if (content == lower(GetKindName(Kind::ExitLoop)))
        kind = Kind::ExitLoop;

    else if (content == lower(GetKindName(Kind::For)))
        kind = Kind::For;

    else if (content == lower(GetKindName(Kind::To)))
        kind = Kind::To;

    else if (content == lower(GetKindName(Kind::In)))
        kind = Kind::In;

    else if (content == lower(GetKindName(Kind::Step)))
        kind = Kind::Step;

    else if (content == lower(GetKindName(Kind::Next)))
        kind = Kind::Next;

    else if (content == lower(GetKindName(Kind::Func)))
        kind = Kind::Func;

    else if (content == lower(GetKindName(Kind::Return)))
        kind = Kind::Return;

    else if (content == lower(GetKindName(Kind::EndFunc)))
        kind = Kind::EndFunc;

    else if (content == lower(GetKindName(Kind::If)))
        kind = Kind::If;

    else if (content == lower(GetKindName(Kind::Then)))
        kind = Kind::Then;

    else if (content == lower(GetKindName(Kind::ElseIf)))
        kind = Kind::ElseIf;

    else if (content == lower(GetKindName(Kind::Else)))
        kind = Kind::Else;

    else if (content == lower(GetKindName(Kind::EndIf)))
        kind = Kind::EndIf;

    else if (content == lower(GetKindName(Kind::Null)))
        kind = Kind::Null;

    else if (content == lower(GetKindName(Kind::Select)))
        kind = Kind::Select;

    else if (content == lower(GetKindName(Kind::Case)))
        kind = Kind::Case;

    else if (content == lower(GetKindName(Kind::EndSelect)))
        kind = Kind::EndSelect;

    else if (content == lower(GetKindName(Kind::Static)))
        kind = Kind::Static;

    else if (content == lower(GetKindName(Kind::Switch)))
        kind = Kind::Switch;

    else if (content == lower(GetKindName(Kind::EndSwitch)))
        kind = Kind::EndSwitch;

    else if (content == lower(GetKindName(Kind::While)))
        kind = Kind::While;

    else if (content == lower(GetKindName(Kind::WEnd)))
        kind = Kind::WEnd;

    else if (content == lower(GetKindName(Kind::With)))
        kind = Kind::With;

    else if (content == lower(GetKindName(Kind::EndWith)))
        kind = Kind::EndWith;


    return Token(kind, m_line, m_begin, start, length);
}

Token Lexer::MakeMultiline() noexcept
{
    const char* start = m_cursor;
    GetNext();

    return Token(Kind::Multiline, m_line, m_begin, start, m_cursor);
   
}

Token Lexer::MakeDecimals() noexcept
{
    const char* start = m_cursor;
    GetNext();

    while (is_digit(GetCurrent())) GetNext();

    return Token(Kind::Decimals, m_line, m_begin, start, m_cursor);
}

Token Lexer::MakeHex() noexcept
{
    const char* start = m_cursor;
    GetNext();

    if (GetCurrent() == 'x' || GetCurrent() == 'X') {
        GetNext();
        while (is_hex_digit(GetCurrent())) GetNext();
        return Token(Kind::Hex, m_line, m_begin, start, m_cursor);
    }
    else
    {
        while (is_digit(GetCurrent())) GetNext();
        return Token(Kind::Decimals, m_line, m_begin, start, m_cursor);
    }   
}

Token Lexer::MakeCommand() noexcept 
{
    const char* start = m_cursor;
    GetNext();

    while (GetCurrent() != '\0')
    {
        if (GetCurrent() == '\n')
        {
            return Token(Kind::AutoItCommand, m_line, m_begin, start, m_cursor);
        }
        GetNext();
    }

    return Token(Kind::Error, m_line, m_begin, m_cursor, 1);
}

Token Lexer::MakeVariable() noexcept
{
    const char* start = m_cursor;
    GetNext();

    while (is_identifier_char(GetCurrent())) GetNext();

    return Token(Kind::Variable, m_line, m_begin, start, m_cursor);
}

Token Lexer::MakeObject() noexcept
{
    const char* start = m_cursor;
    GetNext();

    while (is_identifier_char(GetCurrent())) GetNext();

    return Token(Kind::Object, m_line, m_begin, start, m_cursor);
}

Token Lexer::MakeMacro() noexcept
{
    const char* start = m_cursor;
    GetNext();

    while (is_identifier_char(GetCurrent())) GetNext();

    return Token(Kind::Macro, m_line, m_begin, start, m_cursor);
}


Token Lexer::MakeComment() noexcept 
{
    const char* start = m_cursor;
    GetNext();

    while (GetCurrent() != '\0')
    {
        if (GetCurrent() == '\n')
        {
            return Token(Kind::Comment, m_line, m_begin, start, m_cursor);
        }
        GetNext();
    }

    return Token(Kind::Error, m_line, m_begin, m_cursor, 1);
}

Token Lexer::MakeString(char endsymbol) noexcept
{
    const char* start = m_cursor;
    GetNext();

    while (GetCurrent() != '\0')
    {
        if (GetNext() == endsymbol)
        {
            return Token(Kind::String, m_line, m_begin, start, m_cursor);
        }
    }

    return Token(Kind::Error, m_line, m_begin, m_cursor, 1);
}



std::string readFileIntoString(const std::string& path) {
    std::ifstream input_file(path);

    if (!input_file.is_open()) {
        std::cerr << "Could not open the file - '" << path << "'\n";
        exit(EXIT_FAILURE);
    }
    return std::string((std::istreambuf_iterator<char>(input_file)), std::istreambuf_iterator<char>());
}

FunctionParameter::FunctionParameter(Lexer& lex)
{
    auto curr = lex.Current();

    while (curr.IsNot(Kind::Variable))
    {
        switch (curr.GetKind())
        {
        case Kind::Const:
            m_const = true;
            break;
        case Kind::ByRef:
            m_byref = true;
            break;
        default:
            break;
        }

        curr = lex.Next();
    }

    m_name = curr.GetContent();

    while (true) // lex.Next().IsNot(Kind::Comma))
    {
        curr = lex.Next();

        if (curr.Is(Kind::Equal))
            break;

        if (curr.IsOneOf(Kind::Comma, Kind::ClosedParen))
            return;
    }

    size_t paren_indent = 0;
    //std::cout << "Found Value!\n";

    while (true)
    {
        curr = lex.Next();
        //std::cout << "Current = " << curr.GetName() << "\n";

        if (curr.Is(Kind::OpenedParen))
            paren_indent++;


        if (curr.Is(Kind::Comma) && paren_indent == 0)
            break;

        if (curr.Is(Kind::ClosedParen) && paren_indent == 0)
            break;

        if (curr.Is(Kind::ClosedParen))
            paren_indent--;

        m_default.push_back(curr);
    }

}

Function::Function(Lexer& lex)
{
    // header
    auto curr = lex.Current();
    auto name = lex.Next(Kind::Word);

    m_name = name.GetContent();
    m_start = curr.GetStart();
    m_startline = curr.GetLine();

    lex.Next(Kind::OpenedParen);

    size_t paren_indent = 0;

    while (true)
    {
        auto next = lex.Next();

        if (next.IsOneOf(Kind::Error, Kind::End))
            break;

        if (next.Is(Kind::OpenedParen))
            paren_indent++;

        if (next.Is(Kind::ClosedParen))
        {
            if (paren_indent == 0)
                break;
            paren_indent--;
            continue;
        }

        if (next.IsOneOf(Kind::Const, Kind::ByRef, Kind::Variable))
            m_params.emplace_back(lex);

        if (lex.Current().IsOneOf(Kind::Error, Kind::End, Kind::ClosedParen))
            break;
    }

    auto end = lex.Next(Kind::EndFunc);
    m_end = end.GetEnd();
    m_endline = end.GetLine();
}


int main(int argc, char** argv)
{
    if (argc == 1)
    {
        std::cout << "No AutoIt file parsed!.\n";
        return EXIT_FAILURE;
    }

    auto code = readFileIntoString(argv[1]);

    Lexer lex(code.c_str());

    while (true)
    {
        auto token = lex.Next();
        if (token.IsOneOf(Kind::End, Kind::Error))
            break;

#if 1
        if (token.Is(Kind::Func))
        {            
            auto func = Function(lex);

            std::cout << func.GetStartLine() << ": Func " << func.GetName() << "\n";
            for (size_t i = 0; i < func.GetParamCount(); i++)
            {
                auto param = func.GetParam(i);
                std::cout << "    ";

                if (param.IsConst())
                    std::cout << "Const ";
                if (param.IsByRef())
                    std::cout << "ByRef ";

                std::cout << param.GetName();

                if (param.GetDefaultTokenCount() > 0)
                {
                    std::cout << " =";
                    for (auto val : param.GetDefaultTokens() )
                    {
                        std::cout << val.GetContent();
                    }
                }
                
                std::cout << "\n";
            }

            std::cout << "\n";
        }
#endif

    }

    std::cout << "Lines: " << lex.GetLine() << "\n";
}
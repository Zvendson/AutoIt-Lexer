#pragma once

// Helper functions
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

    std::string lower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
        return s;
    }

}

#define X(identifier) identifier,
#define KINDS \
X(Start             ) \
X(End               ) \
X(Space             ) \
X(LineFeed          ) \
X(Decimals          ) \
X(Hex               ) \
X(False             ) \
X(True              ) \
X(ContinueCase      ) \
X(ContinueLoop      ) \
X(Default           ) \
X(Dim               ) \
X(ReDim             ) \
X(Global            ) \
X(Local             ) \
X(Const             ) \
X(ByRef             ) \
X(Do                ) \
X(Until             ) \
X(Enum              ) \
X(Exit              ) \
X(ExitLoop          ) \
X(For               ) \
X(To                ) \
X(In                ) \
X(Step              ) \
X(Next              ) \
X(Func              ) \
X(Return            ) \
X(EndFunc           ) \
X(If                ) \
X(Then              ) \
X(ElseIf            ) \
X(Else              ) \
X(EndIf             ) \
X(Null              ) \
X(Select            ) \
X(Case              ) \
X(EndSelect         ) \
X(Static            ) \
X(Switch            ) \
X(EndSwitch         ) \
X(While             ) \
X(WEnd              ) \
X(With              ) \
X(EndWith           ) \
X(Word              ) \
X(OpenedParen       ) \
X(ClosedParen       ) \
X(OpenedSquare      ) \
X(ClosedSquare      ) \
X(LessThan          ) \
X(GreaterThan       ) \
X(Equal             ) \
X(Plus              ) \
X(Minus             ) \
X(Asterisk          ) \
X(Slash             ) \
X(Power             ) \
X(AutoItCommand     ) \
X(Variable          ) \
X(Object            ) \
X(Multiline         ) \
X(Comma             ) \
X(Colon             ) \
X(String            ) \
X(Comment           ) \
X(MultiComment      ) \
X(Concatenate       ) \
X(Questionmark      ) \
X(Macro             ) \
X(Error             ) \
X(EnumEnd           ) \

enum class Kind
{
    KINDS
};

#undef X
#define X(identifier) #identifier,

const char* GetKindName(Kind kind)
{
    char const* kind_names[] =
    {
        KINDS
    };

    return kind_names[static_cast<int>(kind)];
}
#undef X

class Token
{
private:
    Kind             m_kind{};
    std::string_view m_content{};
    size_t           m_start = 0;
    size_t           m_end = 0;
    size_t           m_line = 1;

public:
    Token(Kind kind, size_t line, const char* begin, const char* cursor, std::size_t len) noexcept
    {
        m_line = line;
        m_kind = kind;
        m_content = { cursor, len };
        m_start = static_cast<std::size_t>(cursor - begin);
        m_end = m_start + len;
    }

    Token(Kind kind, size_t line, const char* begin, const char* cursor, const char* end) noexcept
    {
        m_line = line;
        m_kind = kind;
        m_start = static_cast<std::size_t>(cursor - begin);
        m_end = static_cast<std::size_t>(end - begin);
        m_content = { cursor, (m_end - m_start) };
    }

    Kind GetKind() const noexcept { return m_kind; }
    const char* GetName() const noexcept { return GetKindName(m_kind); }
    size_t GetStart() const noexcept { return m_start; }
    size_t GetEnd() const noexcept { return m_end; }
    size_t GetLine() const noexcept { return m_line; }

    bool Is(Kind kind) const noexcept { return m_kind == kind; }

    bool IsNot(Kind kind) const noexcept { return m_kind != kind; }

    bool IsOneOf(Kind k1, Kind k2) const noexcept { return Is(k1) || Is(k2); }

    template <typename... Ts>
    bool IsOneOf(Kind k1, Kind k2, Ts... ks) const noexcept
    {
        return Is(k1) || IsOneOf(k2, ks...);
    }

    std::string_view GetContent() const noexcept { return m_content; }
};

class Lexer
{
public:
    Lexer(const char* begin) noexcept : m_begin{ begin }, m_cursor { begin } {}

    //Todo: Previous
    Token Next() noexcept;
    Token Next(Kind) noexcept;
    Token Current() noexcept { return m_current; };
    size_t GetLine() noexcept { return m_line; }

private:
    Token MakeSpace() noexcept;
    Token MakeKeyword(std::string) noexcept;
    Token MakeIdentifier() noexcept;
    Token MakeMultiline() noexcept;
    Token MakeDecimals() noexcept;
    Token MakeHex() noexcept;
    Token MakeCommand() noexcept;
    Token MakeVariable() noexcept;
    Token MakeObject() noexcept;
    Token MakeComment() noexcept;
    Token MakeString(char) noexcept;
    Token MakeMacro() noexcept;
    Token MakeSingle(Kind) noexcept;
    Token MakeError() noexcept;

    char GetCurrent() const noexcept { return *m_cursor; }
    char GetNext() noexcept { return *m_cursor++; }
    char PeekNext() noexcept { 
        char next = *++m_cursor;
        m_cursor--;
        return next;
    }

    const char* m_begin = nullptr;
    const char* m_cursor = nullptr;
    size_t      m_line = 1;
    Token       m_current{Kind::Start, 1, nullptr, nullptr, nullptr};
};


class FunctionParameter
{
private:
    std::string_view   m_name{};
    size_t             m_start = 0;
    size_t             m_end = 0;
    bool               m_const = false;
    bool               m_byref = false;
    std::vector<Token> m_default{};

public:
    FunctionParameter(Lexer&);
    std::string_view GetName() { return m_name; }
    bool IsConst() { return m_const; }
    bool IsByRef() { return m_byref; }
    std::vector<Token> GetDefaultTokens() { return m_default; }
    size_t GetDefaultTokenCount() { return m_default.size(); }
    Token GetDefaultToken(size_t index) { return m_default[index]; }
};

class Function
{
private:
    std::string_view   m_name{};
    size_t             m_start = 0;
    size_t             m_end = 0;
    size_t             m_startline = 1;
    size_t             m_endline = 2;
    std::vector<FunctionParameter> m_params{};
    std::vector<Token> m_content{};

public:
    Function(Lexer&);

    std::string_view GetName() { return m_name; }
    size_t GetStartLine() const noexcept { return m_startline; }
    size_t GetEndLine() const noexcept { return m_endline; }
    std::vector<FunctionParameter> GetParams() { return m_params; }
    size_t GetParamCount() { return m_params.size(); }
    FunctionParameter GetParam(size_t index) { return m_params[index]; }
};



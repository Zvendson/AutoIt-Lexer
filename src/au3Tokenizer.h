#pragma once



namespace Au3
{

    enum class Kind
    {
        Start,
        End,
        Space,
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
        Error,
    };



    class Token
    {
    public:
        Token(Kind kind, size_t line, const char* begin, const char* cursor, std::size_t len) noexcept
        {
            m_Line = line;
            m_Kind = kind;
            m_Content = {cursor, len};
            m_Start = static_cast<std::size_t>(cursor - begin);
            m_End = static_cast<std::size_t>(m_Start + len);
        }

        Token(Kind kind, size_t line, const char* begin, const char* cursor, const char* end) noexcept
        {
            m_Line = line;
            m_Kind = kind;
            m_Start = static_cast<std::size_t>(cursor - begin);
            m_End = static_cast<std::size_t>(end - begin);
            m_Content = { cursor, (m_End - m_Start) };
        }

        Kind               GetKind()    const noexcept { return m_Kind; }
        size_t             GetStart()   const noexcept { return m_Start; }
        size_t             GetEnd()     const noexcept { return m_End; }
        size_t             GetLine()    const noexcept { return m_Line; }
        const std::string& GetContent() const noexcept { return m_Content; }

        bool Is(Kind kind)             const noexcept { return m_Kind == kind; }
        bool IsNot(Kind kind)          const noexcept { return m_Kind != kind; }
        bool IsOneOf(Kind k1, Kind k2) const noexcept { return Is(k1) || Is(k2); }

        template <typename... Ts>
        bool IsOneOf(Kind k1, Kind k2, Ts... ks) const noexcept
        {
            return Is(k1) || IsOneOf(k2, ks...);
        }

    private:
        Kind        m_Kind {};
        std::string m_Content {};
        size_t      m_Start = 0;
        size_t      m_End = 0;
        size_t      m_Line = 1;
    };



    class Tokenizer
    {
    public:
        Tokenizer(const char* begin) noexcept;

        //Todo: Previous
        Token  Next()     noexcept;
        Token  Next(Kind) noexcept;
        Token  Current()  noexcept { return m_Current; };
        size_t GetLine()  noexcept { return m_Line; }

    protected:
        Token MakeSpace()        noexcept;
        Token MakeIdentifier()   noexcept;
        Token MakeMultiline()    noexcept;
        Token MakeDecimals()     noexcept;
        Token MakeHex()          noexcept;
        Token MakeCommand()      noexcept;
        Token MakeVariable()     noexcept;
        Token MakeObject()       noexcept;
        Token MakeComment()      noexcept;
        Token MakeMultiComment() noexcept;
        Token MakeString(char)   noexcept;
        Token MakeMacro()        noexcept;
        Token MakeSingle(Kind)   noexcept;
        Token MakeError()        noexcept;

        char        GetCurrent()     const noexcept { return *m_Cursor; }
        char        GetNext()              noexcept { return *m_Cursor++; }
        char        PeekNext()             noexcept { return *(m_Cursor + 1); }
        char        Peek(size_t len)       noexcept { return *(m_Cursor + len); }
        std::string PeekLine()             noexcept;
        std::string NextLine()             noexcept;

    protected:
        const char* m_Begin   = nullptr;
        const char* m_Cursor  = nullptr;
        const char* m_End     = nullptr;
        size_t      m_Line    = 1;
        Token       m_Current = { Kind::Start, 1, nullptr, nullptr, nullptr };
    };

}









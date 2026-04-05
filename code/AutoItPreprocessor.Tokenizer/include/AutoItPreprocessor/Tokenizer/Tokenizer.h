#pragma once

#include "AutoItPreprocessor/Tokenizer/Token.h"

#include <cstddef>
#include <string>
#include <vector>

namespace AutoItPreprocessor::Tokenizer
{
    class Tokenizer
    {
    public:
        explicit Tokenizer(std::string sourceText);

        [[nodiscard]] Token Peek();
        [[nodiscard]] Token Next();
        [[nodiscard]] Token Current() const noexcept { return m_Current; }
        [[nodiscard]] std::size_t GetLine() const noexcept { return m_Line; }
        [[nodiscard]] std::vector<Token> TokenizeAll();
        void Reset() noexcept;

    private:
        Token MakeSpace();
        Token MakeTab();
        Token MakeIdentifier();
        Token MakeMultiline();
        Token MakeDecimals();
        Token MakeHex();
        Token MakeCommand();
        Token MakeVariable();
        Token MakeObject();
        Token MakeComment();
        Token MakeMultiComment();
        Token MakeString(char endSymbol);
        Token MakeMacro();
        Token MakeSingle(TokenKind kind);
        Token MakeError();
        std::string PeekLine() const;
        std::string NextLine();

        char GetCurrent() const noexcept;
        char GetNext() noexcept;
        char PeekNextChar() const noexcept;

        std::string m_SourceText;
        const char* m_Begin = nullptr;
        const char* m_Cursor = nullptr;
        const char* m_End = nullptr;
        std::size_t m_Line = 1;
        Token m_Current = {};
    };
}

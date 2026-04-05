#include "SymbolAnalysis.h"

#include "EditorServices.h"

#include "AutoItPreprocessor/Tokenizer/Token.h"
#include "AutoItPreprocessor/Tokenizer/Tokenizer.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <map>
#include <set>
#include <unordered_map>

namespace
{
    using AutoItPreprocessor::Tokenizer::Token;
    using AutoItPreprocessor::Tokenizer::TokenKind;

    struct ParsedFileSymbols
    {
        std::vector<AutoItPlus::Editor::FunctionSymbol> functions;
        std::vector<AutoItPlus::Editor::VariableSymbol> globals;
        std::vector<AutoItPlus::Editor::VariableSymbol> constants;
        std::vector<AutoItPlus::Editor::IncludeSymbol> includes;
        std::set<std::string> localFunctions;
        std::set<std::string> localGlobals;
        std::set<std::string> usedFunctions;
        std::set<std::string> usedVariables;
    };

    std::string TrimCopy(const std::string& text)
    {
        const auto begin = std::find_if_not(text.begin(), text.end(), [](unsigned char ch) { return std::isspace(ch) != 0; });
        const auto end = std::find_if_not(text.rbegin(), text.rend(), [](unsigned char ch) { return std::isspace(ch) != 0; }).base();
        if (begin >= end)
            return {};
        return std::string(begin, end);
    }

    bool IsTrivia(TokenKind kind)
    {
        return kind == TokenKind::Space
            || kind == TokenKind::Tab
            || kind == TokenKind::LineFeed
            || kind == TokenKind::Comment
            || kind == TokenKind::MultiComment;
    }

    bool IsDeclarationKeyword(const Token& token)
    {
        if (!token.Is(TokenKind::Keyword))
            return false;

        const auto value = AutoItPreprocessor::Tokenizer::ToLowerCopy(token.GetContent());
        return value == "local" || value == "global" || value == "dim" || value == "const" || value == "static";
    }

    bool IsInsideFunction(std::size_t functionDepth)
    {
        return functionDepth > 0;
    }

    std::vector<AutoItPlus::Editor::IncludeSymbol> ParseIncludes(const std::string& text)
    {
        std::vector<AutoItPlus::Editor::IncludeSymbol> includes;
        std::istringstream stream(text);
        std::string line;
        int lineNumber = 1;
        while (std::getline(stream, line))
        {
            const std::string trimmed = TrimCopy(line);
            const std::string lowered = AutoItPreprocessor::Tokenizer::ToLowerCopy(trimmed);
            if (!lowered.starts_with("#include"))
            {
                ++lineNumber;
                continue;
            }

            const std::string payload = TrimCopy(trimmed.substr(8));
            if (payload.size() >= 2U && payload.front() == '"' && payload.back() == '"')
                includes.push_back({payload.substr(1, payload.size() - 2U), lineNumber, false});
            else if (payload.size() >= 2U && payload.front() == '<' && payload.back() == '>')
                includes.push_back({payload.substr(1, payload.size() - 2U), lineNumber, true});

            ++lineNumber;
        }

        return includes;
    }

    ParsedFileSymbols ParseSymbols(const std::string& text)
    {
        ParsedFileSymbols result;
        result.includes = ParseIncludes(text);
        AutoItPreprocessor::Tokenizer::Tokenizer tokenizer(text);
        const auto tokens = tokenizer.TokenizeAll();

        std::size_t functionDepth = 0;
        AutoItPlus::Editor::FunctionSymbol* currentFunction = nullptr;
        bool expectFunctionName = false;
        bool expectFunctionParameters = false;
        bool parsingFunctionParameters = false;
        int parameterParenDepth = 0;
        bool expectDeclVariable = false;
        bool declarationIsConst = false;
        bool declarationIsStatic = false;

        for (std::size_t index = 0; index < tokens.size(); ++index)
        {
            const auto& token = tokens[index];
            if (token.Is(TokenKind::End) || token.Is(TokenKind::Error) || IsTrivia(token.GetKind()))
                continue;

            const auto lowered = AutoItPreprocessor::Tokenizer::ToLowerCopy(token.GetContent());
            if (token.Is(TokenKind::Keyword) && lowered == "func")
            {
                expectFunctionName = true;
                ++functionDepth;
                continue;
            }

            if (token.Is(TokenKind::Keyword) && lowered == "endfunc")
            {
                if (functionDepth > 0)
                    --functionDepth;
                if (functionDepth == 0)
                    currentFunction = nullptr;
                continue;
            }

            if (expectFunctionName && token.Is(TokenKind::Word))
            {
                result.functions.push_back({token.GetContent(), static_cast<int>(token.GetLine()), {}, {}});
                currentFunction = &result.functions.back();
                result.localFunctions.insert(AutoItPreprocessor::Tokenizer::ToLowerCopy(token.GetContent()));
                expectFunctionName = false;
                expectFunctionParameters = true;
                continue;
            }

            if (expectFunctionParameters)
            {
                if (token.Is(TokenKind::OpenedParen))
                {
                    parsingFunctionParameters = true;
                    parameterParenDepth = 1;
                    expectFunctionParameters = false;
                    continue;
                }

                if (!token.Is(TokenKind::Word) && !token.Is(TokenKind::Space) && !token.Is(TokenKind::Tab))
                    expectFunctionParameters = false;
            }

            if (parsingFunctionParameters)
            {
                if (token.Is(TokenKind::OpenedParen))
                {
                    ++parameterParenDepth;
                    continue;
                }

                if (token.Is(TokenKind::ClosedParen))
                {
                    --parameterParenDepth;
                    if (parameterParenDepth <= 0)
                        parsingFunctionParameters = false;
                    continue;
                }

                if (parameterParenDepth == 1 && token.Is(TokenKind::Variable) && currentFunction != nullptr)
                    currentFunction->parameters.push_back({token.GetContent(), static_cast<int>(token.GetLine())});

                continue;
            }

            if (IsDeclarationKeyword(token))
            {
                expectDeclVariable = true;
                declarationIsConst = lowered == "const";
                declarationIsStatic = lowered == "static";
                continue;
            }

            if (expectDeclVariable)
            {
                if (token.Is(TokenKind::Variable))
                {
                    AutoItPlus::Editor::VariableSymbol symbol{token.GetContent(), static_cast<int>(token.GetLine())};
                    if (IsInsideFunction(functionDepth) && currentFunction != nullptr)
                        currentFunction->locals.push_back(symbol);
                    else if (declarationIsConst)
                        result.constants.push_back(symbol);
                    else
                        result.globals.push_back(symbol);
                    if (!IsInsideFunction(functionDepth) || declarationIsStatic)
                        result.localGlobals.insert(AutoItPreprocessor::Tokenizer::ToLowerCopy(token.GetContent()));
                    continue;
                }

                if (token.Is(TokenKind::Comma))
                    continue;

                expectDeclVariable = false;
                declarationIsConst = false;
                declarationIsStatic = false;
            }

            if (token.Is(TokenKind::Word))
            {
                std::size_t nextIndex = index + 1U;
                while (nextIndex < tokens.size() && IsTrivia(tokens[nextIndex].GetKind()))
                    ++nextIndex;

                if (nextIndex < tokens.size() && tokens[nextIndex].Is(TokenKind::OpenedParen))
                    result.usedFunctions.insert(AutoItPreprocessor::Tokenizer::ToLowerCopy(token.GetContent()));
            }
            else if (token.Is(TokenKind::Variable))
            {
                result.usedVariables.insert(AutoItPreprocessor::Tokenizer::ToLowerCopy(token.GetContent()));
            }
        }

        return result;
    }
}

namespace AutoItPlus::Editor
{
    OutlineData AnalyzeOutline(
        const std::optional<OutlineProjectContext>&,
        const std::filesystem::path&,
        const std::string& text)
    {
        OutlineData outline;
        const auto currentSymbols = ParseSymbols(text);
        outline.functions = currentSymbols.functions;
        outline.globals = currentSymbols.globals;
        outline.constants = currentSymbols.constants;
        outline.includes = currentSymbols.includes;

        return outline;
    }

    OutlineData AnalyzeOutline(const EditorState& state, const DocumentState& document)
    {
        std::optional<OutlineProjectContext> projectContext;
        if (state.project.has_value())
            projectContext = OutlineProjectContext{ state.project->rootDirectory };

        return AnalyzeOutline(projectContext, document.path, document.editor->GetText());
    }
}

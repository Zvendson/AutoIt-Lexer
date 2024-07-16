#include <filesystem>
#include <iostream>
#include <fstream>
#include <au3Tokenizer.h>


const char* GetKindName(Au3::Kind kind)
{
    switch (kind)
    {
        case Au3::Kind::Start:
            return "Start";

        case Au3::Kind::End:
            return "End";

        case Au3::Kind::Space:
            return "Space";

        case Au3::Kind::LineFeed:
            return "LineFeed";

        case Au3::Kind::Decimals:
            return "Decimals";

        case Au3::Kind::Hex:
            return "Hex";

        case Au3::Kind::Keyword:
            return "Keyword";

        case Au3::Kind::Word:
            return "Word";

        case Au3::Kind::OpenedParen:
            return "OpenedParen";

        case Au3::Kind::ClosedParen:
            return "ClosedParen";

        case Au3::Kind::OpenedSquare:
            return "OpenedSquare";

        case Au3::Kind::ClosedSquare:
            return "ClosedSquare";

        case Au3::Kind::LessThan:
            return "LessThan";

        case Au3::Kind::GreaterThan:
            return "GreaterThan";

        case Au3::Kind::Equal:
            return "Equal";

        case Au3::Kind::Plus:
            return "Plus";

        case Au3::Kind::Minus:
            return "Minus";

        case Au3::Kind::Asterisk:
            return "Asterisk";

        case Au3::Kind::Slash:
            return "Slash";

        case Au3::Kind::Power:
            return "Power";

        case Au3::Kind::AutoItCommand:
            return "AutoItCommand";

        case Au3::Kind::Variable:
            return "Variable";

        case Au3::Kind::Object:
            return "Object";

        case Au3::Kind::Multiline:
            return "Multiline";

        case Au3::Kind::Comma:
            return "Comma";

        case Au3::Kind::Colon:
            return "Colon";

        case Au3::Kind::String:
            return "String";

        case Au3::Kind::Comment:
            return "Comment";

        case Au3::Kind::MultiComment:
            return "MultiComment";

        case Au3::Kind::Concatenate:
            return "Concatenate";

        case Au3::Kind::Questionmark:
            return "Questionmark";

        case Au3::Kind::Macro:
            return "Macro";

        case Au3::Kind::Error:
            return "Error";

        default:
            return "UnknownKind";
    }
}


// https://stackoverflow.com/questions/5878775/how-to-find-and-replace-string
void replace_all(std::string& s, std::string const& toReplace, std::string const& replaceWith)
{
    std::string buf;
    std::size_t pos = 0;
    std::size_t prevPos;

    // Reserves rough estimate of final size of string.
    buf.reserve(s.size());

    while (true) 
    {
        prevPos = pos;
        pos = s.find(toReplace, pos);
        if (pos == std::string::npos)
            break;
        
        buf += s.substr(prevPos, pos - prevPos);
        buf += replaceWith;
        pos += toReplace.size();
    }

    buf.append(s, prevPos, s.size() - prevPos);
    s.swap(buf);
}



std::string readFileIntoString(const std::string& path)
{
    std::ifstream input_file(path);

    if (!input_file.is_open()) 
    {
        std::cerr << "Could not open the file - '" << path << "'\n";
        exit(EXIT_FAILURE);
    }

    return std::string((std::istreambuf_iterator<char>(input_file)), std::istreambuf_iterator<char>());
}



int main(int argc, char** argv)
{
    std::string code = {};

    switch (argc)
    {
        case 0:
        case 1:
            {
                std::filesystem::path cwd = std::filesystem::current_path() / "Memory.au3";
                code = readFileIntoString(cwd.string());
            }
            break;

        default:
            code = readFileIntoString(argv[1]);
            break;
    }

    Au3::Tokenizer tokenizer(code.c_str());




    // just some test / playground of how it could be used.

    std::vector<std::vector<Au3::Token>> functions;

    while (true)
    {
        auto token = tokenizer.Next();
        if (token.Is(Au3::Kind::Keyword) && token.GetContentLower() == "func")
        {
            std::vector<Au3::Token> function;
            do
            {
                function.push_back(token);
                if (token.Is(Au3::Kind::Keyword) && token.GetContentLower() == "endfunc")
                {
                    functions.push_back(function);
                    break;
                }

                if (token.IsOneOf(Au3::Kind::End, Au3::Kind::Error))
                    break;

                token = tokenizer.Next();

            } while (true);

        }

        if (token.IsOneOf(Au3::Kind::End, Au3::Kind::Error))
            break;
    }
    std::cout << "Lines: " << tokenizer.GetLine() << "\n";
    std::cout << "Functions:\n";
    
    for (auto function : functions)
    {
        for (auto token : function)
        {
            std::cout << token.GetContent();
        }
        std::cout << "\n\n";
    }
}

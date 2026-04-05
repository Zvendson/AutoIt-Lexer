#include "AutoItPreprocessor/Compiler/Compiler.h"
#include "AutoItPreprocessor/Tokenizer/Token.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
    struct CommandLine
    {
        std::string command;
        std::filesystem::path inputFile;
        std::filesystem::path outputFile;
        std::vector<std::filesystem::path> includeDirs;
        std::vector<std::filesystem::path> customFiles;
    };

    std::string Escape(const std::string& content)
    {
        std::string escaped;
        for (const char c : content)
        {
            switch (c)
            {
                case '\n': escaped += "\\n"; break;
                case '\r': escaped += "\\r"; break;
                case '\t': escaped += "\\t"; break;
                default: escaped += c; break;
            }
        }

        return escaped;
    }

    void PrintUsage()
    {
        std::cout
            << "Usage:\n"
            << "  AutoItPreprocessor tokenize <input.au3> [--include-dir <dir>] [--custom <rules.tokens>]\n"
            << "  AutoItPreprocessor strip    <input.au3> [--out <output.au3>] [--include-dir <dir>]\n"
            << "  AutoItPreprocessor compile  <input.au3> --out <output.au3> [--include-dir <dir>] [--custom <rules.tokens>]\n";
    }

    CommandLine ParseArguments(int argc, char** argv)
    {
        if (argc < 3)
            throw std::runtime_error("Not enough arguments.");

        CommandLine commandLine;
        commandLine.command = argv[1];
        commandLine.inputFile = argv[2];

        for (int index = 3; index < argc; ++index)
        {
            const std::string arg = argv[index];
            if (arg == "--include-dir")
            {
                if (++index >= argc)
                    throw std::runtime_error("Missing path after --include-dir");
                commandLine.includeDirs.emplace_back(argv[index]);
            }
            else if (arg == "--custom")
            {
                if (++index >= argc)
                    throw std::runtime_error("Missing path after --custom");
                commandLine.customFiles.emplace_back(argv[index]);
            }
            else if (arg == "--out")
            {
                if (++index >= argc)
                    throw std::runtime_error("Missing path after --out");
                commandLine.outputFile = argv[index];
            }
            else
            {
                throw std::runtime_error("Unknown argument: " + arg);
            }
        }

        return commandLine;
    }

    void WriteOutput(const std::filesystem::path& outputFile, const std::string& text)
    {
        if (outputFile.has_parent_path())
            std::filesystem::create_directories(outputFile.parent_path());

        std::ofstream output(outputFile);
        if (!output.is_open())
            throw std::runtime_error("Could not write output file: " + outputFile.string());

        output << text;
    }

    std::filesystem::path MakeStrippedOutputPath(const std::filesystem::path& inputFile)
    {
        const auto extension = inputFile.has_extension() ? inputFile.extension().string() : std::string(".au3");
        return inputFile.parent_path() / (inputFile.stem().string() + "_stripped" + extension);
    }
}

int main(int argc, char** argv)
{
    try
    {
        const auto commandLine = ParseArguments(argc, argv);

        AutoItPreprocessor::Compiler::Compiler compiler;
        AutoItPreprocessor::Compiler::CompilerOptions options;
        options.includeDirectories = commandLine.includeDirs;
        options.customRuleFiles = commandLine.customFiles;

        const auto compilation = compiler.Compile(commandLine.inputFile, options);

        if (commandLine.command == "tokenize")
        {
            for (const auto& token : compilation.tokens)
            {
                std::cout
                    << std::setw(6) << token.GetLine() << "  "
                    << std::setw(16) << AutoItPreprocessor::Tokenizer::ToString(token.GetKind()) << "  "
                    << Escape(token.Is(AutoItPreprocessor::Tokenizer::TokenKind::Custom) ? token.GetReplacement() : token.GetContent())
                    << '\n';
            }

            return EXIT_SUCCESS;
        }

        if (commandLine.command == "strip")
        {
            const auto outputPath = commandLine.outputFile.empty() ? MakeStrippedOutputPath(commandLine.inputFile) : commandLine.outputFile;
            WriteOutput(outputPath, compilation.strippedCode);
            std::cout << "Wrote " << outputPath.string() << '\n';
            return EXIT_SUCCESS;
        }

        if (commandLine.command == "compile")
        {
            if (commandLine.outputFile.empty())
                throw std::runtime_error("compile requires --out <output.au3>");

            WriteOutput(commandLine.outputFile, compilation.generatedCode);
            std::cout << "Wrote " << commandLine.outputFile.string() << '\n';
            return EXIT_SUCCESS;
        }

        throw std::runtime_error("Unknown command: " + commandLine.command);
    }
    catch (const std::exception& exception)
    {
        std::cerr << exception.what() << '\n';
        PrintUsage();
        return EXIT_FAILURE;
    }
}

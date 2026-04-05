#pragma once

#include <filesystem>
#include <string>

namespace AutoItPreprocessor::Common
{
    struct SourceDocument
    {
        std::filesystem::path path;
        std::string text;
    };
}

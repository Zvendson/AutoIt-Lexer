add_library(AutoItPreprocessor.Common STATIC
    src/SourceDocument.cpp
)

target_include_directories(AutoItPreprocessor.Common
    PUBLIC
        include
)

autoit_apply_warnings(AutoItPreprocessor.Common)

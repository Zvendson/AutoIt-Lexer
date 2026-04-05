add_library(AutoItPreprocessor.Tokenizer STATIC
    src/Token.cpp
    src/Tokenizer.cpp
)

target_include_directories(AutoItPreprocessor.Tokenizer
    PUBLIC
        include
)

target_link_libraries(AutoItPreprocessor.Tokenizer
    PUBLIC
        AutoItPreprocessor.Common
)

autoit_apply_warnings(AutoItPreprocessor.Tokenizer)

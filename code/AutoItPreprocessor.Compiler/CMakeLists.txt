add_library(AutoItPreprocessor.Compiler STATIC
    src/Compiler.cpp
    src/CustomTokenRegistry.cpp
    src/Emitter.cpp
    src/IncludeResolver.cpp
)

target_include_directories(AutoItPreprocessor.Compiler
    PUBLIC
        include
)

target_link_libraries(AutoItPreprocessor.Compiler
    PUBLIC
        AutoItPreprocessor.Common
        AutoItPreprocessor.Tokenizer
)

autoit_apply_warnings(AutoItPreprocessor.Compiler)

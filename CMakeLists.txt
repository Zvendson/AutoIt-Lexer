cmake_minimum_required(VERSION 3.20)

project(Torii VERSION 1.0 LANGUAGES CXX)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(BUILD_TESTING ON CACHE BOOL "Enable tests" FORCE)
endif()

include(CTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

function(autoit_apply_warnings target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /MP /W4 /permissive-)
    else()
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

add_subdirectory(code/AutoItPreprocessor.Common)
add_subdirectory(code/AutoItPreprocessor.Tokenizer)
add_subdirectory(code/AutoItPreprocessor.Compiler)
add_subdirectory(code/AutoItPreprocessor.Cli)
add_subdirectory(code/AutoItPreprocessor.Editor)

if(BUILD_TESTING)
    add_test(
        NAME tokenize_sample
        COMMAND AutoItPreprocessor tokenize "${CMAKE_SOURCE_DIR}/tests/data/root.au3" --include-dir "${CMAKE_SOURCE_DIR}/tests/data/includes"
    )

    add_test(
        NAME strip_sample
        COMMAND AutoItPreprocessor strip
            "${CMAKE_SOURCE_DIR}/tests/data/root.au3"
            --out "${CMAKE_BINARY_DIR}/generated/root_stripped.au3"
            --include-dir "${CMAKE_SOURCE_DIR}/tests/data/includes"
    )

    add_test(
        NAME compile_sample
        COMMAND AutoItPreprocessor compile
            "${CMAKE_SOURCE_DIR}/tests/data/root.au3"
            --out "${CMAKE_BINARY_DIR}/generated/compiled-root.au3"
            --include-dir "${CMAKE_SOURCE_DIR}/tests/data/includes"
            --custom "${CMAKE_SOURCE_DIR}/tests/data/custom.tokens"
    )
endif()

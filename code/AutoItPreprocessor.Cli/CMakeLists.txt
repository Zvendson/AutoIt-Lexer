add_executable(AutoItPreprocessor
    src/main.cpp
)

target_link_libraries(AutoItPreprocessor
    PRIVATE
        AutoItPreprocessor.Compiler
)

foreach(config Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER "${config}" config_upper)
    set_target_properties(AutoItPreprocessor PROPERTIES
        "RUNTIME_OUTPUT_DIRECTORY_${config_upper}" "${CMAKE_SOURCE_DIR}/bin/${config}/AutoItPreprocessor"
    )
endforeach()

set_target_properties(AutoItPreprocessor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/AutoItPreprocessor"
)

autoit_apply_warnings(AutoItPreprocessor)

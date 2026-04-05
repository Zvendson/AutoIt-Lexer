set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

set(TORII_LABS_VERSION "${PROJECT_VERSION}")
set(TORII_LABS_VERSION_LABEL "${PROJECT_VERSION} Alpha")
string(REPLACE "." "," TORII_LABS_VERSION_COMMA "${TORII_LABS_VERSION}")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/Version.h"
    @ONLY
)
if(WIN32)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/Version.rc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/generated/Version.rc"
        @ONLY
    )
endif()

set(AUTOIT_DEPS_DIR "${CMAKE_SOURCE_DIR}/deps")
set(glfw_SOURCE_DIR "${AUTOIT_DEPS_DIR}/glfw")
set(imgui_SOURCE_DIR "${AUTOIT_DEPS_DIR}/imgui")

option(AUTOIT_FETCH_EDITOR_DEPS "Fetch missing editor dependencies automatically during configure." ON)

if(NOT EXISTS "${glfw_SOURCE_DIR}" OR NOT EXISTS "${imgui_SOURCE_DIR}")
    if(NOT AUTOIT_FETCH_EDITOR_DEPS)
        foreach(dep_dir
            "${glfw_SOURCE_DIR}"
            "${imgui_SOURCE_DIR}"
        )
            if(NOT EXISTS "${dep_dir}")
                message(FATAL_ERROR "Missing dependency directory: ${dep_dir}. Initialize the submodules in deps/ or enable AUTOIT_FETCH_EDITOR_DEPS.")
            endif()
        endforeach()
    endif()

    include(FetchContent)
    cmake_policy(PUSH)
    cmake_policy(SET CMP0169 OLD)

    if(NOT EXISTS "${glfw_SOURCE_DIR}")
        FetchContent_Declare(glfw
            GIT_REPOSITORY https://github.com/glfw/glfw.git
            GIT_TAG 3.4
        )
        FetchContent_MakeAvailable(glfw)
    endif()

    if(NOT EXISTS "${imgui_SOURCE_DIR}")
        FetchContent_Declare(imgui
            GIT_REPOSITORY https://github.com/ocornut/imgui.git
            GIT_TAG v1.91.9b
        )
        FetchContent_Populate(imgui)
        set(imgui_SOURCE_DIR "${imgui_SOURCE_DIR}" CACHE PATH "" FORCE)
    endif()

    cmake_policy(POP)
else()
    foreach(dep_dir
        "${glfw_SOURCE_DIR}"
        "${imgui_SOURCE_DIR}"
    )
        if(NOT EXISTS "${dep_dir}")
            message(FATAL_ERROR "Missing dependency directory: ${dep_dir}. Initialize the submodules in deps/.")
        endif()
    endforeach()
endif()

if(NOT TARGET glfw)
    add_subdirectory("${glfw_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/deps/glfw")
endif()

add_library(AutoItPreprocessor.ImGui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_widgets/ConsoleWidget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_widgets/TextEditor.cpp
)

target_include_directories(AutoItPreprocessor.ImGui
    PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
        ${imgui_SOURCE_DIR}/misc/cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_widgets
)

target_link_libraries(AutoItPreprocessor.ImGui
    PUBLIC
        glfw
)

find_package(OpenGL REQUIRED)
target_link_libraries(AutoItPreprocessor.ImGui PUBLIC OpenGL::GL)

autoit_apply_warnings(AutoItPreprocessor.ImGui)

if(MSVC)
    target_compile_options(AutoItPreprocessor.ImGui PRIVATE /wd4189 /wd4244)
endif()

add_executable(ToriiLabs
    src/AutoItSyntax.cpp
    src/EditorServices.cpp
    src/HotkeyManager.cpp
    src/settings/ProjectSettingsFile.cpp
    src/settings/ShortcutSettingsFile.cpp
    src/settings/SettingFile.cpp
    src/settings/UserSettingsFile.cpp
    src/settings/WorkspaceSettingsFile.cpp
    src/imgui_widgets/SocialLinkWidgets.cpp
    src/EditorUi.cpp
    src/SymbolAnalysis.cpp
    src/main.cpp
)

if(WIN32)
    target_sources(ToriiLabs PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/Version.rc")
endif()

target_link_libraries(ToriiLabs
    PRIVATE
        AutoItPreprocessor.Compiler
        AutoItPreprocessor.ImGui
        OpenGL::GL
)

if(WIN32)
    target_link_libraries(ToriiLabs PRIVATE windowscodecs ole32)
endif()

target_include_directories(ToriiLabs
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_widgets
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party
        ${CMAKE_CURRENT_BINARY_DIR}/generated
)

foreach(config Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER "${config}" config_upper)
    set_target_properties(ToriiLabs PROPERTIES
        "RUNTIME_OUTPUT_DIRECTORY_${config_upper}" "${CMAKE_SOURCE_DIR}/bin/${config}/ToriiLabs"
    )
endforeach()

set_target_properties(ToriiLabs PROPERTIES
    OUTPUT_NAME "Torii Labs"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/ToriiLabs"
)

autoit_apply_warnings(ToriiLabs)

if(MSVC)
    target_compile_options(ToriiLabs PRIVATE /wd4005)
endif()

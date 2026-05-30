cmake_minimum_required(VERSION 4.2.0)
project(
    BiTUI
    VERSION 0.1.0
    DESCRIPTION "BiTUI - Bit level fast Terminal-User-Interface"
    LANGUAGES CXX
)

option(BITUI_BUILD_EXAMPLES "Build BiTUI examples" ON)
option(BITUI_BUILD_TESTS "Build BiTUI tests" OFF)

# ----------------------------------------------------------------------

add_library(BiTUI_compiler_warnings INTERFACE)
add_library(BiTUI::compiler_warnings ALIAS BiTUI_compiler_warnings)

target_compile_options(
    BiTUI_compiler_warnings
    INTERFACE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /constexpr:steps20000000>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wshadow -Wnon-virtual-dtor -Wpedantic -Werror>
)

# ----------------------------------------------------------------------

add_executable(BiTUI)
# add_library(BiTUI STATIC)
# add_library(BiTUI::BiTUI ALIAS BiTUI)

target_sources(
    BiTUI
    PRIVATE
    src/main.cpp
    src/ansi.cpp
    src/ScreenBuffer.cpp
    src/Renderer.cpp
    src/Terminal.cpp
)

target_include_directories(
    BiTUI
    PUBLIC
    include
)

target_link_libraries(
    BiTUI
    PRIVATE
    BiTUI::compiler_warnings
)

if(MSVC)
    target_compile_options(
        BiTUI 
        PUBLIC 
        /std:c++latest 
        /permissive-
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_features(
        BiTUI
        PUBLIC
        cxx_std_26
    )
endif()

# ----------------------------------------------------------------------

option(ENABLE_CLANG_TIDY "Enable clang-tidy analysis during build" OFF)

if(ENABLE_CLANG_TIDY)
    if(NOT CMAKE_CXX_CLANG_TIDY)
        find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
        if(CLANG_TIDY_EXE)
            set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
        endif()
    endif()

    if(CMAKE_CXX_CLANG_TIDY)
        message(STATUS "Using clang-tidy: ${CMAKE_CXX_CLANG_TIDY}")
        list(APPEND CMAKE_CXX_CLANG_TIDY "-extra-arg=-Wno-unknown-warning-option")
    else()
        message(WARNING "clang-tidy not found, disabling ENABLE_CLANG_TIDY")
    endif()
endif()

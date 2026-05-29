cmake_minimum_required(VERSION 4.2.0)
project(
    BiTUI
    VERSION 0.1.0
    DESCRIPTION "BiTUI - Bit level fast Terminal-User-Interface"
    LANGUAGES CXX
)

option(BITUI_BUILD_EXAMPLES "Build BiTUI examples" ON)
option(BITUI_BUILD_TESTS "Build BiTUI tests" OFF)

add_executable(BiTUI)
# add_library(BiTUI STATIC)
# add_library(${PROJECT_NAME}::BiTUI ALIAS BiTUI)

target_sources(
    BiTUI
    PRIVATE
    src/main.cpp
    src/ansi.cpp
    src/ScreenBuffer.cpp
    src/Renderer.cpp
)

target_include_directories(
    BiTUI
    PUBLIC
    include
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



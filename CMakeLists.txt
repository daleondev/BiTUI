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
)

target_compile_features(
    BiTUI
    PUBLIC
    cxx_std_26
)

# CMakeLists.txt
# T# v2.0.0-beta1
# Dylan Armstrong, 2026

cmake_minimum_required(VERSION 3.20)

project(tsharp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_definitions(-D_USE_MATH_DEFINES)

find_package(
    antlr4-runtime
    REQUIRED
    CONFIG
)

# Include all C++ source files recursively,
# including src/bytecode/*.cpp.
file(
    GLOB_RECURSE
    TSHARP_SRC
    CONFIGURE_DEPENDS

    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

add_executable(
    tsharp

    main.cpp

    ${TSHARP_SRC}

    generated/grammar/TSharpLexer.cpp
    generated/grammar/TSharpParser.cpp
    generated/grammar/TSharpVisitor.cpp
    generated/grammar/TSharpBaseVisitor.cpp
)

target_include_directories(
    tsharp
    PRIVATE

    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/runtime_model
    ${CMAKE_CURRENT_SOURCE_DIR}/include/bytecode

    ${CMAKE_CURRENT_SOURCE_DIR}/generated
    ${CMAKE_CURRENT_SOURCE_DIR}/generated/grammar
)

target_link_libraries(
    tsharp
    PRIVATE

    antlr4_shared
)

if(MSVC)
    target_compile_options(
        tsharp
        PRIVATE

        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:Release>:/GL>
    )

    target_link_options(
        tsharp
        PRIVATE

        $<$<CONFIG:Release>:/LTCG>
    )
else()
    target_compile_options(
        tsharp
        PRIVATE

        $<$<CONFIG:Release>:-O3>
    )
endif()

include(CheckIPOSupported)

check_ipo_supported(
    RESULT IPO_SUPPORTED
    OUTPUT IPO_ERROR
)

if(IPO_SUPPORTED)
    set_property(
        TARGET tsharp
        PROPERTY
        INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE
    )
endif()
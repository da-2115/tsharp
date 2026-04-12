cmake_minimum_required(VERSION 3.20)
project(tsharp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-D_USE_MATH_DEFINES)

# Find ANTLR4 via vcpkg
find_package(antlr4-runtime REQUIRED CONFIG)

# T# sources
file(GLOB TSHARP_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

# Executable
add_executable(tsharp
    main.cpp
    ${TSHARP_SRC}
    generated/grammar/TSharpLexer.cpp
    generated/grammar/TSharpParser.cpp
    generated/grammar/TSharpVisitor.cpp
    generated/grammar/TSharpBaseVisitor.cpp
)

target_include_directories(tsharp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/generated
    ${CMAKE_CURRENT_SOURCE_DIR}/generated/grammar
)

target_link_libraries(tsharp PRIVATE antlr4_shared)
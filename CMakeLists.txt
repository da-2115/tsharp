cmake_minimum_required(VERSION 3.20)
project(tsharp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(antlr4-runtime REQUIRED)

file(GLOB SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

add_executable(tsharp
    main.cpp
    ${SRC_FILES}
    generated/grammar/TSharpLexer.cpp
    generated/grammar/TSharpParser.cpp
)

target_include_directories(tsharp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/generated
    ${CMAKE_CURRENT_SOURCE_DIR}/generated/grammar
)

target_link_libraries(tsharp PRIVATE antlr4_shared)
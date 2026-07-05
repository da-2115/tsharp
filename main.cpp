// main.cpp
// Dylan Armstrong, 2026

#include "Interpreter.h"

#include <cstring>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

constexpr std::string_view tsharp_version = "v1.0.0-beta5";

int main(int argc, const char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: tsharp <file1.tsharp> [file2.tsharp ...]\n";
        return 1;
    }

    if (std::strcmp(argv[1], "--version") == 0) {
        std::cout << "The T# Programming Language " << tsharp_version << "\nWritten by Dylan Armstrong, 2026\n";
        return 0;
    }

    std::vector<std::string> files;

    for (int i = 1; i < argc; ++i) {
        files.push_back(argv[i]);
    }

    try {
        tsharp::Interpreter interpreter;
        interpreter.load_program_files(files);
    }

    catch (const tsharp::RuntimeError &e) {
        std::cerr << "Runtime error: " << e.what() << '\n';
        return 2;
    }

    catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 3;
    }

    return 0;
}
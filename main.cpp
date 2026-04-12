// main.cpp
// Dylan Armstrong, 2026

#include "Interpreter.h"
#include "antlr4-runtime.h"
#include "TSharpLexer.h"
#include "TSharpParser.h"
#include <fstream>
#include <iostream>
#include <sstream>

// T# version string
constexpr const char* tsharp_version = "v1.0.0-alpha3";

// Main function of T#
int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: tsharp <file.tsharp> OR tsharp --version\n";
        return 1;
    }

    if (strcmp(argv[1], "--version") == 0) {
        std::cout << "The T# Progamming Language " << tsharp_version << std::endl << "Written by Dylan Armstrong, 2026" << std::endl;
        return 0;
    }

    std::ifstream input(argv[1]);
    if (!input) {
        std::cerr << "Could not open file: " << argv[1] << "\n";
        return 1;
    }

    std::stringstream buffer;
    buffer << input.rdbuf();

    antlr4::ANTLRInputStream stream(buffer.str());
    TSharpLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    TSharpParser parser(&tokens);
    parser.removeErrorListeners();
    parser.addErrorListener(new antlr4::ConsoleErrorListener());

    auto* tree = parser.program();
    tsharp::Interpreter interpreter;

    try {
        interpreter.execute(tree);
    } catch (const tsharp::RuntimeError& e) {
        std::cerr << "Runtime error: " << e.what() << '\n';
        return 2;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 3;
    }

    return 0;
}

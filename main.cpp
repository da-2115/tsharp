// main.cpp
// Dylan Armstrong, 2026

#include "Interpreter.h"
#include "antlr4-runtime.h"
#include "TSharpLexer.h"
#include "TSharpParser.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string_view>

// T# version string
constexpr std::string_view tsharp_version = "v1.0.0-beta3";

struct ParsedFile {
    std::string source;
    std::unique_ptr<antlr4::ANTLRInputStream> stream;
    std::unique_ptr<TSharpLexer> lexer;
    std::unique_ptr<antlr4::CommonTokenStream> tokens;
    std::unique_ptr<TSharpParser> parser;
    TSharpParser::ProgramContext* tree = nullptr;
};

// Main function of T#
int main(int argc, const char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: tsharp <file1.tsharp> [file2.tsharp ...]" << std::endl;
        return 1;
    }

    if (strcmp(argv[1], "--version") == 0) {
        std::cout << "The T# Progamming Language " << tsharp_version
                  << std::endl
                  << "Written by Dylan Armstrong, 2026"
                  << std::endl;
        return 0;
    }

    std::vector<std::string> files;
    for (int i = 1; i < argc; ++i) {
        files.push_back(argv[i]);
    }

    tsharp::Interpreter interpreter;
std::vector<std::unique_ptr<ParsedFile>> parsed_files;

    try {
        for (const auto& file : files) {
            std::ifstream input(file);

            if (!input) {
                std::cerr << "Could not open file: " << file << "\n";
                return 1;
            }

        auto parsed = std::make_unique<ParsedFile>();

        std::stringstream buffer;
        buffer << input.rdbuf();
        parsed->source = buffer.str();

        parsed->stream = std::make_unique<antlr4::ANTLRInputStream>(parsed->source);
        parsed->lexer = std::make_unique<TSharpLexer>(parsed->stream.get());
        parsed->tokens = std::make_unique<antlr4::CommonTokenStream>(parsed->lexer.get());
        parsed->parser = std::make_unique<TSharpParser>(parsed->tokens.get());

        parsed->parser->removeErrorListeners();
        parsed->parser->addErrorListener(new antlr4::ConsoleErrorListener());

        parsed->tree = parsed->parser->program();

        interpreter.load(parsed->tree);

        parsed_files.push_back(std::move(parsed));
    }

    interpreter.run_main();
}
    catch (const tsharp::RuntimeError& e) {
        std::cerr << "Runtime error: " << e.what() << '\n';
        return 2;
    }

    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 3;
    }

    return 0;
}
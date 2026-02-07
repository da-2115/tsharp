// main.cpp
// Dylan Armstrong, 2026

#include <iostream>
#include <fstream>

#include "antlr4-runtime.h"
#include "tsharp_lexer.h"
#include "tsharp_parser.h"
#include "tsharp_listener.h"

int main(int argc, const char* argv[]) {
    // Check for command line argument (with file name path)
    if (argc < 2) {
        std::cerr << "Usage: tsharp <file>" << std::endl;
        return 1;
    }

    // Open file stream with .tsharp file
    std::shared_ptr<std::ifstream> stream = std::make_unique<std::ifstream>(argv[1]);
    if (!stream->is_open()) {
        std::cerr << "Error: Cannot open file " << argv[1] << std::endl;
        return 1;
    }
    
    antlr4::ANTLRInputStream input(*stream);
    tsharp_lexer lexer(&input);
    antlr4::CommonTokenStream tokens(&lexer);
    tsharp_parser parser(&tokens);
    
    antlr4::tree::ParseTree* tree = parser.program();
    tsharp_listener listener;
    antlr4::tree::ParseTreeWalker::DEFAULT.walk(&listener, tree);

    return 0;
}
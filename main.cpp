// main.cpp
// Dylan Armstrong, 2026

#include <chrono>
#include <fstream>
#include <iostream>

#include "antlr4-runtime.h"
#include "tsharp_lexer.h"
#include "tsharp_listener.h"
#include "tsharp_parser.h"

int main(int argc, const char* argv[]) {
	// Check for command line argument (with file name path)
	if (argc < 2) {
		std::cerr << "Usage: tsharp <file>" << std::endl;
		return 1;
	}

	// Open file stream with .tsharp file
	std::unique_ptr<std::ifstream> stream = std::make_unique<std::ifstream>(argv[1]);
	if (!stream->is_open()) {
		std::cerr << "Error: Cannot open file " << argv[1] << std::endl;
		return 1;
	}

	antlr4::ANTLRInputStream input(*stream);
	tsharp_lexer lexer(&input);
	antlr4::CommonTokenStream tokens(&lexer);
	tsharp_parser parser(&tokens);
	
	antlr4::tree::ParseTree* tree = parser.program();
	
	if (parser.getNumberOfSyntaxErrors() > 0) {
		std::cerr << "WARNING: Parser encountered syntax errors!" << std::endl;
	}
	
	if (!tree) {
		std::cerr << "ERROR: Parse tree is null!" << std::endl;
		return 1;
	}
	
	tsharp_listener listener;
	
	try {
		antlr4::tree::ParseTreeWalker::DEFAULT.walk(&listener, tree);
	} catch (const std::exception& e) {
		std::cerr << "ERROR: Exception during parse tree walk: " << e.what() << std::endl;
		return 1;
	} catch (...) {
		std::cerr << "ERROR: Unknown exception during parse tree walk" << std::endl;
		return 1;
	}

	return 0;
}
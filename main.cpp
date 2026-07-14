// main.cpp
// T# v2.0.0-beta1
// Dylan Armstrong, 2026

#include "TSharpLexer.h"
#include "TSharpParser.h"

#include "ModuleLoader.h"

#include "Compiler.h"
#include "VM.h"

#include <antlr4-runtime.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

// Version string
constexpr std::string_view tsharp_version = "v2.0.0-beta1";

namespace {

std::string read_file_text(const std::filesystem::path& file_path) {
	std::ifstream file(file_path);

	if (!file.is_open()) {
		throw std::runtime_error("Could not open file: " + file_path.string());
	}

	std::stringstream buffer;
	buffer << file.rdbuf();

	return buffer.str();
}

int run_bytecode(const std::filesystem::path& input_path) {
	tsharp::ModuleLoader loader(".");

	tsharp::ParsedModule* entry = loader.load_entry(input_path);

	if (entry == nullptr || entry->program == nullptr) {
		throw std::runtime_error("Failed to load entry module: " + input_path.string());
	}

	// Contains the entry module and every recursively imported module
	const auto programs = loader.programs();

	tsharp::Compiler compiler;

	tsharp::BytecodeModule module = compiler.compile(programs, entry->program);

	tsharp::VM vm;
	vm.run(module);

	return 0;
}

// int run_interpreter(const std::filesystem::path& input_path) {
// 	const std::string source = read_file_text(input_path);

// 	antlr4::ANTLRInputStream input(source);
// 	TSharpLexer lexer(&input);
// 	antlr4::CommonTokenStream tokens(&lexer);
// 	TSharpParser parser(&tokens);

// 	TSharpParser::ProgramContext* program = parser.program();

// 	if (parser.getNumberOfSyntaxErrors() > 0) {
// 		std::cerr << "Compilation failed due to syntax errors.\n";

// 		return 2;
// 	}

// 	tsharp::Interpreter interpreter;
// 	interpreter.execute(program);

// 	return 0;
// }

}

int main(int argc, const char* argv[]) {
	if (argc < 2) {
		std::cerr << "Usage: tsharp <file.tsharp>\n";

		return 1;
	}

	if (std::strcmp(argv[1], "--version") == 0) {
		std::cout << "The T# Programming Language " << tsharp_version << "\n"
				  << "Written by Dylan Armstrong, 2026\n";

		return 0;
	}

	try {
		const std::filesystem::path file_path = std::filesystem::absolute(argv[1]);

		return run_bytecode(file_path);
	} catch (const tsharp::RuntimeError& e) {
		std::cerr << "Runtime error: " << e.what() << '\n';

		return 2;
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << '\n';

		return 3;
	}
}
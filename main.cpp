// main.cpp
// T# v2.1.1
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

constexpr std::string_view tsharp_version = "v2.1.1";

namespace {

struct CommandLineOptions {
	bool verbose = false;
	bool trace = false;
	bool bytecode = false;
};

std::string read_file_text(const std::filesystem::path& file_path) {
	std::ifstream file(file_path);

	if (!file.is_open()) {
		throw std::runtime_error("Could not open file: " + file_path.string());
	}

	std::stringstream buffer;
	buffer << file.rdbuf();

	return buffer.str();
}

int run_bytecode(const std::filesystem::path& input_path, const CommandLineOptions& options) {
	tsharp::ModuleLoader loader(".");

	tsharp::ParsedModule* entry = loader.load_entry(input_path);

	if (entry == nullptr || entry->program == nullptr) {
		throw std::runtime_error("Failed to load entry module: " + input_path.string());
	}

	const auto programs = loader.programs();

	tsharp::Compiler compiler;

	tsharp::BytecodeModule module = compiler.compile(programs, entry->program);

	/*
	if (options.bytecode) {
		tsharp::disassemble_module(module);
	}
	*/

	tsharp::VM vm({.trace = options.trace});

	vm.run(module);

	return 0;
}

void print_usage() {
	std::cout << "Usage: tsharp [options] <file.tsharp>\n"
			  << '\n'
			  << "Options:\n"
			  << "  -v, --verbose    Enable bytecode output and VM tracing\n"
			  << "      --trace      Enable VM instruction tracing\n"
			  << "      --bytecode   Display compiled bytecode\n"
			  << "      --version    Display version information\n";
}

}

int main(int argc, const char* argv[]) {
	if (argc < 2) {
		print_usage();

		return 1;
	}

	CommandLineOptions options;

	std::filesystem::path file_path;

	for (int i = 1; i < argc; i++) {
		const std::string argument = argv[i];

		if (argument == "--version") {
			std::cout << "The T# Programming Language " << tsharp_version << "\n"
					  << "Written by Dylan Armstrong, 2026\n";

			return 0;
		}

		if (argument == "--verbose" || argument == "-v") {
			options.verbose = true;
			options.trace = true;
			options.bytecode = true;

			continue;
		}

		if (argument == "--trace") {
			options.trace = true;

			continue;
		}

		if (argument == "--bytecode") {
			options.bytecode = true;

			continue;
		}

		if (!argument.empty() && argument[0] == '-') {
			std::cerr << "Unknown option: " << argument << '\n';

			return 1;
		}

		if (!file_path.empty()) {
			std::cerr << "Only one entry file may be specified\n";

			return 1;
		}

		file_path = argument;
	}

	if (file_path.empty()) {
		print_usage();

		return 1;
	}

	try {
		file_path = std::filesystem::absolute(file_path);

		return run_bytecode(file_path, options);
	} catch (const tsharp::RuntimeError& e) {
		std::cerr << "Runtime error: " << e.what() << '\n';

		return 2;
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << '\n';

		return 3;
	}
}
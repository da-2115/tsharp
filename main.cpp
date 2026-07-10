// main.cpp
// T# v1.0.0-betarc (v1.0.0 Beta Release Candidate/Final Beta)
// Dylan Armstrong, 2026

#include "Interpreter.h"

#include <cstring>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

// Compile time constant readonly string for the version string
constexpr std::string_view tsharp_version = "v1.0.0-betarc";

// Main function
int main(int argc, const char* argv[]) {
	// If not enough command line arguments provided
	if (argc < 2) {
		std::cerr << "Usage: tsharp <file1.tsharp> [file2.tsharp ...]\n";
		return 1;
	}

	// If --version is passed as the command line argument, return the about info
	if (std::strcmp(argv[1], "--version") == 0) {
		std::cout << "The T# Programming Language " << tsharp_version << "\nWritten by Dylan Armstrong, 2026\n";
		return 0;
	}

	// Get files passed as command line arguments as strings to their file paths in a std::vector object
	std::vector<std::string> files;

	// Go through all command line arguments and add file paths (as strings) to the files vector
	for (int i = 1; i < argc; ++i) {
		files.push_back(argv[i]);
	}

	// Create an interpreter object
	// Then, load the program files (calls load_program_files method from the interpreter)
	try {
		tsharp::Interpreter interpreter;
		interpreter.load_program_files(files);
	}

	// If something goes wrong (RuntimeError)
	catch (const tsharp::RuntimeError& e) {
		std::cerr << "Runtime error: " << e.what() << '\n';
		return 2;
	}

	// Otherwise, if something else goes wrong
	catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << '\n';
		return 3;
	}

	return 0;
}
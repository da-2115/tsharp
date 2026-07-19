// ModuleLoader.cpp
// T# v2.0.0-beta1
// Dylan Armstrong, 2026

#include "ModuleLoader.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

#include <cctype>

namespace {

std::string trim_copy(const std::string& text) {
	size_t start = 0;

	while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start]))) {
		start++;
	}

	size_t end = text.size();

	while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
		end--;
	}

	return text.substr(start, end - start);
}

}

namespace tsharp {

ModuleLoader::ModuleLoader(std::filesystem::path stdlib_root) : stdlib_root(std::move(stdlib_root)) {
}

ParsedModule* ModuleLoader::load_entry(const std::filesystem::path& path) {
	return load_recursive(path, true);
}

std::vector<TSharpParser::ProgramContext*> ModuleLoader::programs() const {
	std::vector<TSharpParser::ProgramContext*> result;

	result.reserve(modules.size());

	for (const auto& module : get_modules()) {
		result.push_back(module->program);
	}

	return result;
}

ParsedModule* ModuleLoader::load_recursive(const std::filesystem::path& path, bool) {
	const auto absolute_path = std::filesystem::weakly_canonical(path);

	const std::string key = canonical_key(absolute_path);

	if (auto it = loaded.find(key); it != loaded.end()) {
		return it->second;
	}

	if (loading.contains(key)) {
		throw std::runtime_error("Circular import detected involving: " + absolute_path.string());
	}

	if (!std::filesystem::exists(absolute_path)) {
		throw std::runtime_error("Module not found: " + absolute_path.string());
	}

	loading.insert(key);

	auto module = std::make_unique<ParsedModule>();

	module->path = absolute_path;
	module->source = read_file(absolute_path);

	module->input = std::make_unique<antlr4::ANTLRInputStream>(module->source);

	module->lexer = std::make_unique<TSharpLexer>(module->input.get());

	module->tokens = std::make_unique<antlr4::CommonTokenStream>(module->lexer.get());

	module->parser = std::make_unique<TSharpParser>(module->tokens.get());

	module->program = module->parser->program();

	if (module->parser->getNumberOfSyntaxErrors() != 0) {
		throw std::runtime_error("Syntax errors in module: " + absolute_path.string());
	}

	ParsedModule* result = module.get();

	// Register before loading children so duplicate imports
	// return the same parsed module.
	loaded[key] = result;
	modules.push_back(std::move(module));

	for (auto* import_decl : result->program->importDecl()) {
		const std::string import_name = trim_copy(import_decl->getText().substr(std::string("import").size()));
		const auto import_path = resolve_import(import_name, absolute_path);

		load_recursive(import_path, false);
	}

	loading.erase(key);

	return result;
}

std::filesystem::path ModuleLoader::resolve_import(const std::string& import_name, const std::filesystem::path& importer) const {
	std::string relative = import_name;

	for (char& c : relative) {
		if (c == '.') {
			c = std::filesystem::path::preferred_separator;
		}
	}

	relative += ".tsharp";

	// First allow imports relative to the current source file.
	const auto local = importer.parent_path() / relative;

	if (std::filesystem::exists(local)) {
		return local;
	}

	// Then standard library.
	const auto standard = stdlib_root / relative;

	if (std::filesystem::exists(standard)) {
		return standard;
	}

	throw std::runtime_error("Could not resolve import '" + import_name + "' from " + importer.string());
}

std::string ModuleLoader::read_file(const std::filesystem::path& path) {
	std::ifstream stream(path);

	if (!stream) {
		throw std::runtime_error("Unable to open file: " + path.string());
	}

	std::ostringstream buffer;
	buffer << stream.rdbuf();

	return buffer.str();
}

std::string ModuleLoader::canonical_key(const std::filesystem::path& path) {
	return std::filesystem::weakly_canonical(path).generic_string();
}

}
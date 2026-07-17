// ModuleLoader.h
// T# v2.0.0-beta1
// Dylan Armstrong, 2026

#pragma once

#include "TSharpLexer.h"
#include "TSharpParser.h"

#include <antlr4-runtime.h>

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace tsharp {

// ParsedModule struct
struct ParsedModule {
	std::filesystem::path path;
	std::string source;

	std::unique_ptr<antlr4::ANTLRInputStream> input;
	std::unique_ptr<TSharpLexer> lexer;
	std::unique_ptr<antlr4::CommonTokenStream> tokens;
	std::unique_ptr<TSharpParser> parser;

	TSharpParser::ProgramContext* program = nullptr;
};

// ModuleLoader class
// Loads modules from the std library
class ModuleLoader {
  private:
	ParsedModule* load_recursive(const std::filesystem::path& path, bool is_entry);

	std::filesystem::path resolve_import(const std::string& import_name, const std::filesystem::path& importer) const;

	static std::string read_file(const std::filesystem::path& path);

	static std::string canonical_key(const std::filesystem::path& path);

	std::filesystem::path stdlib_root;

	std::vector<std::unique_ptr<ParsedModule>> modules;

	std::unordered_map<std::string, ParsedModule*> loaded;
	std::unordered_set<std::string> loading;

  public:
	explicit ModuleLoader(std::filesystem::path stdlib_root);

	ParsedModule* load_entry(const std::filesystem::path& path);

	const std::vector<std::unique_ptr<ParsedModule>>& get_modules() const {
		return modules;
	}

	std::vector<TSharpParser::ProgramContext*> programs() const;
};

}
// BytecodeModule.h
// T# v2.0.0-beta1
// Dylan Armstrong, 2026

#pragma once

#include "BytecodeFunction.h"
#include "Value.h"

#include <cstddef>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace tsharp {

// Native function type alias
using NativeFunction = std::function<Value(const std::vector<Value>&)>;

// Native function information
struct NativeFunctionInfo {
	std::string name;

	size_t arity = 0;

	NativeFunction function;
};

// Enum information
struct EnumInfo {
	std::string name;

	std::unordered_map<std::string, int> values;
};

// Bytecode field information
struct BytecodeFieldInfo {
	std::string name;

	size_t slot = 0;
};

// Class information
struct ClassInfo {
	std::string name;

	std::string base_class;

	// Abstract class?
	bool is_abstract = false;

	std::vector<BytecodeFieldInfo> fields;

	std::unordered_map<std::string, size_t> field_slots;

	std::unordered_map<std::string, size_t> methods;

	std::unordered_map<std::string, size_t> properties;

	std::vector<size_t> constructors;
};

// Bytecode Module
// A module of bytecode which contains information about functions, classes, enums, etc.
struct BytecodeModule {
	std::vector<BytecodeFunction> functions;

	std::unordered_map<std::string, size_t> function_lookup;

	std::vector<ClassInfo> classes;

	std::unordered_map<std::string, size_t> class_lookup;

	std::vector<EnumInfo> enums;

	std::unordered_map<std::string, size_t> enum_lookup;

	std::vector<Value> globals;

	std::unordered_map<std::string, size_t> global_lookup;

	std::vector<NativeFunctionInfo> natives;

	std::unordered_map<std::string, size_t> native_lookup;

	std::vector<std::string> interfaces;

	size_t main_function = 0;
};

}
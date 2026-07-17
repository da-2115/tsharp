// BytecodeFunction.h
// T# v2.0.0-beta1
// Dylan Armstrong, 2026

#pragma once

#include "Chunk.h"

#include <cstddef>
#include <string>
#include <vector>

namespace tsharp {

// Bytecode for T# functions
struct BytecodeFunction {
	// Function name
	std::string name;

	Chunk chunk;

	size_t arity = 0;
	size_t local_count = 0;

	// Is it a method or a constructor (part of an object)?
	bool is_method = false;
	bool is_constructor = false;

	// Parameter information
	std::vector<std::string> parameter_names;
	std::vector<std::string> parameter_types;
};

}
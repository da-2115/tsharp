// FunctionValue.h
// Dylan Armstrong, 2026

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "Environment.h"
#include "Value.h"

#include "ParameterInfo.h"

namespace tsharp {

// Function value struct
// Stores information about T# functions
struct FunctionValue {
	std::string name;

	// Parameters and retrun type
	std::vector<ParameterInfo> params;
	std::string return_type;

	// What type of function is it?
	bool is_native = false;
	bool is_method = false;
	bool is_static = false;
	bool is_virtual = false;
	bool is_override = false;
	bool is_abstract = false;
	bool is_private = false;
	bool is_protected = false;
	bool is_public = true;

	// Body of the function
	void* body_node = nullptr;

	std::shared_ptr<Environment> closure;
	std::function<Value(const std::vector<Value>&, const Value&)> native;
};

}
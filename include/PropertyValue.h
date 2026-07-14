// PropertyValue.h
// Dylan Armstrong, 2026

#pragma once

#include <string>

namespace tsharp {

// Property value struct
// Used for properties
struct PropertyValue {
	std::string name;
	std::string type_name;
	bool is_arrow = false;
	bool is_private = false;
	bool is_protected = false;
	bool is_public = true;
	void* body_node = nullptr;
	void* expr_node = nullptr;
};

}
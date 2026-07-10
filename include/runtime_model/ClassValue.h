// ClassValue.h
// T# v1.0.0-betarc (v1.0.0 Beta Release Candidate/Final Beta)
// Dylan Armstrong, 2026

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "FieldInfo.h"
#include "FunctionValue.h"
#include "MemberInfo.h"
#include "PropertyValue.h"
#include "Value.h"

namespace tsharp {
// Forward declaration for interpreter class - which implements the visitor pattern logic
class Interpreter;

// Class value struct
// Contains information about classes in T#
struct ClassValue {
	std::string name;

	// For generic types and generic programming e.g. class<T>
	std::vector<std::string> generic_names;

	// Base class name - for child classes that inherit a parent class
	std::string base_class_name;

	// Booleans
	// These determine whether the class is an interface, abstract class or an enum
	bool is_interface = false;
	bool is_abstract = false;
	bool is_enum = false;

	// Class fields
	std::unordered_map<std::string, Value> static_fields;
	std::unordered_map<std::string, Value> field_defaults;

	// Class methods, constructors, properties
	std::unordered_map<std::string, std::shared_ptr<FunctionValue>> methods;
	std::unordered_map<size_t, std::shared_ptr<FunctionValue>> constructors;
	std::unordered_map<std::string, PropertyValue> properties;

	std::unordered_map<std::string, FieldInfo> field_metadata;
	std::unordered_map<std::string, MemberInfo> member_lookup;
	std::vector<std::string> interfaces;
};

}
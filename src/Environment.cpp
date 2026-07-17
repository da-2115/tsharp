// Environment.cpp
// Dylan Armstrong, 2026

#include "Environment.h"

namespace tsharp {

// Environment constructor
Environment::Environment(std::shared_ptr<Environment> parent) : parent(std::move(parent)) {
}

// Define environment
void Environment::define(const std::string& name, const Value& value) {
	values.emplace(name, value);
}

// Assign environment
void Environment::assign(const std::string& name, const Value& value) {
	Value* destination = find_mutable(name);

	if (!destination) {
		throw RuntimeError("Undefined variable: " + name);
	}

	*destination = value;
}

// Get environment value
Value Environment::get(const std::string& name) const {
	const Value* value = find(name);

	if (value) {
		return *value;
	}

	throw RuntimeError("Undefined variable: " + name);
}

const Value& Environment::get_ref(const std::string& name) const {
	const Value* value = find(name);

	if (!value) {
		throw RuntimeError("Undefined variable: " + name);
	}

	return *value;
}

const Value* Environment::find(const std::string& name) const {
	auto it = values.find(name);

	if (it != values.end()) {
		return &it->second;
	}

	if (parent) {
		return parent->find(name);
	}

	return nullptr;
}

Value* Environment::find_mutable(const std::string& name) {
	auto it = values.find(name);

	if (it != values.end()) {
		return &it->second;
	}

	if (parent) {
		return parent->find_mutable(name);
	}

	return nullptr;
}

// Does a local exist with the same name?
bool Environment::exists_local(const std::string& name) const {
	return values.contains(name);
}

std::shared_ptr<Environment> Environment::get_parent() const {
	return parent;
}

}

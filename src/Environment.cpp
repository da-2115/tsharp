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
	if (values.contains(name)) {
		values.at(name) = value;

		return;
	}

	if (parent) {
		parent->assign(name, value);

		return;
	}

	throw RuntimeError("Undefined variable: " + name);
}

// Get environment value
Value Environment::get(const std::string& name) const {
	auto it = values.find(name);

	if (it != values.end()) {
		return it->second;
	}

	if (parent) {
		return parent->get(name);
	}

	throw RuntimeError("Undefined variable: " + name);
}

// Does a local exist with the same name?
bool Environment::exists_local(const std::string& name) const {
	return values.contains(name);
}

std::shared_ptr<Environment> Environment::get_parent() const {
	return parent;
}

}

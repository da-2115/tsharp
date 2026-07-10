// Environment.h
// T# v1.0.0-betarc (v1.0.0 Beta Release Candidate/Final Beta)
// Dylan Armstrong, 2026

#pragma once

#include "Value.h"
#include <memory>
#include <string>
#include <unordered_map>

namespace tsharp {

// Environment class definition
// define T# functions, objects, classes, etc. into the environment
class Environment : public std::enable_shared_from_this<Environment> {
  public:
	// Explicit constructor, default the parent object to a nullptr
	explicit Environment(std::shared_ptr<Environment> parent = nullptr);

	// Define an environment value, assign environment value, get the environment value
	void define(const std::string& name, const Value& value);
	void assign(const std::string& name, const Value& value);
	Value get(const std::string& name) const;

	// Does the current environment value exist already?
	bool exists_local(const std::string& name) const;

	// get the parent environment value of the current value
	std::shared_ptr<Environment> get_parent() const;

  private:
	// values and the parent value
	std::unordered_map<std::string, Value> values;
	std::shared_ptr<Environment> parent;
};

}

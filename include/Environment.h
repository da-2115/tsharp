// Environment.h
// Dylan Armstrong, 2026

#pragma once

#include "Value.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace tsharp {

class Environment : public std::enable_shared_from_this<Environment> {
  public:
	explicit Environment(std::shared_ptr<Environment> parent = nullptr);

	void define(const std::string& name, const Value& value);
	void assign(const std::string& name, const Value& value);

	Value get(const std::string& name) const;
	const Value& get_ref(const std::string& name) const;

	const Value* find(const std::string& name) const;
	Value* find_mutable(const std::string& name);

	bool exists_local(const std::string& name) const;

	std::shared_ptr<Environment> get_parent() const;

  private:
	std::unordered_map<std::string, Value> values;
	std::shared_ptr<Environment> parent;
};

}
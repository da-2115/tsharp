// InstanceValue.h
// Dylan Armstrong, 2026

#pragma once

#include "Value.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

namespace tsharp {

struct InstanceValue : public std::enable_shared_from_this<InstanceValue> {
	std::shared_ptr<ClassValue> class_val;

	std::unordered_map<std::string, Value> fields;

	explicit InstanceValue(std::shared_ptr<ClassValue> c) : class_val(std::move(c)) {
	}
};

}
// InstanceValue.h
// Dylan Armstrong, 2026

#pragma once

#include <memory>
#include <unordered_map>

#include "ClassValue.h"

namespace tsharp {
// Forward declaration for interpreter class - which implements the visitor pattern logic
class Interpreter;

// Instance value struct, inherits parent class
struct InstanceValue : public std::enable_shared_from_this<InstanceValue> {
    std::shared_ptr<ClassValue> class_val;
    std::unordered_map<std::string, Value> fields;

    // Explicit constructor
    explicit InstanceValue(std::shared_ptr<ClassValue> c) : class_val(std::move(c)) {}
};

}
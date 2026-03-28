// RuntimeModel.h
// Dylan Armstrong, 2026

#pragma once

#include "Environment.h"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace tsharp {

// Forward declaration for interpreter class - which implements the visitor pattern logic
class Interpreter;

// Parameter info struct
struct ParameterInfo {
    std::string type_name;
    std::string name;
};

// Function value struct
struct FunctionValue {
    std::string name;
    std::vector<ParameterInfo> params;
    std::string return_type;
    bool is_native = false;
    bool is_method = false;
    bool is_static = false;
    bool is_virtual = false;
    bool is_override = false;
    bool is_abstract = false;
    void* body_node = nullptr;
    std::shared_ptr<Environment> closure;
    std::function<Value(Interpreter&, const std::vector<Value>&, const Value&)> native;
};

// Property value struct
struct PropertyValue {
    std::string name;
    std::string type_name;
    bool is_arrow = false;
    void* body_node = nullptr;
    void* expr_node = nullptr;
};

// Class value struct
struct ClassValue {
    std::string name;
    std::vector<std::string> generic_names;
    std::string base_class_name;
    bool is_interface = false;
    bool is_abstract = false;
    std::unordered_map<std::string, Value> static_fields;
    std::unordered_map<std::string, Value> field_defaults;
    std::unordered_map<std::string, std::shared_ptr<FunctionValue>> methods;
    std::unordered_map<std::string, std::shared_ptr<FunctionValue>> constructors;
    std::unordered_map<std::string, PropertyValue> properties;
    std::vector<std::string> interfaces;
};

// Instance value struct, inherits parent class
struct InstanceValue : public std::enable_shared_from_this<InstanceValue> {
    std::shared_ptr<ClassValue> class_val;
    std::unordered_map<std::string, Value> fields;

    // Explicit constructor
    explicit InstanceValue(std::shared_ptr<ClassValue> c) : class_val(std::move(c)) {}
};

}

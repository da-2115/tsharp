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

// Optimization #4: Control flow enum for replacing exceptions
enum class ControlFlow { NORMAL, RETURN_VALUE, BREAK, CONTINUE };

// Execution result with control flow status (Optimization #4)
struct ExecutionResult {
    Value value;
    ControlFlow flow = ControlFlow::NORMAL;
};

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
    bool is_private = false;
    bool is_protected = false;
    bool is_public = true;
    void* body_node = nullptr;
    std::shared_ptr<Environment> closure;
    std::function<Value(Interpreter&, const std::vector<Value>&, const Value&)> native;
};

// Property value struct
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

// Field info struct for tracking field access modifiers
struct FieldInfo {
    std::string type_name;
    bool is_static = false;
    bool is_private = false;
    bool is_protected = false;
    bool is_public = true;
};

// Member info struct for unified member lookup (optimization #3)
struct MemberInfo {
    enum Type { FIELD, METHOD, PROPERTY } type;
    std::shared_ptr<FunctionValue> method;  // When type == METHOD
    PropertyValue property;                 // When type == PROPERTY
    FieldInfo field_meta;                   // Metadata for all types
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
    std::unordered_map<size_t, std::shared_ptr<FunctionValue>> constructors;  // Optimization #2: Key by arity
    std::unordered_map<std::string, PropertyValue> properties;
    std::unordered_map<std::string, FieldInfo> field_metadata;
    std::unordered_map<std::string, MemberInfo> member_lookup;  // Optimization #3: Unified lookup
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

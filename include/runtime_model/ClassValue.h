// ClassValue.h
// Dylan Armstrong, 2026

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

#include "Value.h"
#include "FunctionValue.h"
#include "PropertyValue.h"
#include "FieldInfo.h"
#include "MemberInfo.h"

namespace tsharp {
// Forward declaration for interpreter class - which implements the visitor pattern logic
class Interpreter;

// Class value struct
struct ClassValue {
    std::string name;
    std::vector<std::string> generic_names;
    std::string base_class_name;
    bool is_interface = false;
    bool is_abstract = false;
    bool is_enum = false;
    std::unordered_map<std::string, Value> static_fields;
    std::unordered_map<std::string, Value> field_defaults;
    std::unordered_map<std::string, std::shared_ptr<FunctionValue>> methods;
    std::unordered_map<size_t, std::shared_ptr<FunctionValue>> constructors;
    std::unordered_map<std::string, PropertyValue> properties;
    std::unordered_map<std::string, FieldInfo> field_metadata;
    std::unordered_map<std::string, MemberInfo> member_lookup; 
    std::vector<std::string> interfaces;
};

}
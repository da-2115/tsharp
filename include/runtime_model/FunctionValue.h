// FunctionValue.h
// Dylan Armstrong, 2026

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>

#include "Environment.h"
#include "Value.h"

#include "ParameterInfo.h"

namespace tsharp {
class Interpreter;

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

}
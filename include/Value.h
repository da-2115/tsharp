// Value.h
// Dylan Armstrong, 2026

#pragma once

#include <any>
#include <cmath>
#include <functional>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

// tsharp namespace declaration
namespace tsharp {

// Forward declarations
struct FunctionValue;
struct ClassValue;
struct InstanceValue;

using Array = std::vector<class Value>;
using ObjectMap = std::unordered_map<std::string, class Value>;

// RuntimeError class - inherits std::runtime_error
class RuntimeError : public std::runtime_error {
public:
    explicit RuntimeError(const std::string& message) : std::runtime_error(message) {}
};

class Value {
public:
    // Variant type alias for std::variant of all T# types
    using Variant = std::variant<std::monostate, int, float, double, bool, char, std::string, std::shared_ptr<Array>, std::shared_ptr<ObjectMap>,
        std::shared_ptr<FunctionValue>, std::shared_ptr<ClassValue>, std::shared_ptr<InstanceValue>, std::any>;
    
    // Constructors for Value
    Value();
    Value(int v);
    Value(float v);
    Value(double v);
    Value(bool v);
    Value(char v);
    Value(const std::string& v);
    Value(const char* v);
    Value(std::shared_ptr<Array> v);
    Value(std::shared_ptr<ObjectMap> v);
    Value(std::shared_ptr<FunctionValue> v);
    Value(std::shared_ptr<ClassValue> v);
    Value(std::shared_ptr<InstanceValue> v);
    Value(std::any v);

    // Bool checking member functions -> is it (type here)?
    bool is_null() const;
    bool is_number() const;
    bool is_int() const;
    bool is_float() const;
    bool is_double() const;
    bool is_bool() const;
    bool is_char() const;
    bool is_string() const;
    bool is_array() const;
    bool is_instance() const;
    bool is_class() const;
    bool is_function() const;

    // Express as type member functions
    double as_double() const;
    float as_float() const;
    int as_int() const;
    char as_char() const;
    bool as_bool() const;
    std::string as_string() const;
    std::shared_ptr<Array> as_array() const;
    std::shared_ptr<InstanceValue> as_instance() const;
    std::shared_ptr<ClassValue> as_class() const;
    std::shared_ptr<FunctionValue> as_function() const;

    // Get raw data getters
    const Variant& raw() const;
    Variant& raw();

private:
    // Value data member variable
    Variant data;
};

// Helper functions
std::string value_type_name(const Value& v);
Value apply_binary_numeric_op(const Value& a, const Value& b, const std::function<double(double, double)>& fn);
bool values_equal(const Value& a, const Value& b);

}

namespace tsharp {
// Signal classes - all inherit std::exception
class ReturnSignal : public std::exception {
public:
    Value value;
    explicit ReturnSignal(const Value& v);
};

class BreakSignal : public std::exception {};
class ContinueSignal : public std::exception {};
class ThrowSignal : public std::exception {
public:
    Value value;
    explicit ThrowSignal(const Value& v);
};

}

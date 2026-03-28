// Value.cpp
// Dylan Armstrong, 2026

#include "Value.h"

#include <iomanip>

namespace tsharp {

// Implementation of constructors
Value::Value() : data(std::monostate{}) {}

Value::Value(int v) : data(v) {}

Value::Value(float v) : data(v) {}

Value::Value(double v) : data(v) {}

Value::Value(bool v) : data(v) {}

Value::Value(char v) : data(v) {}

Value::Value(const std::string& v) : data(v) {}

Value::Value(const char* v) : data(std::string(v ? v : "")) {}

Value::Value(std::shared_ptr<Array> v) : data(std::move(v)) {}

Value::Value(std::shared_ptr<ObjectMap> v) : data(std::move(v)) {}

Value::Value(std::shared_ptr<FunctionValue> v) : data(std::move(v)) {}

Value::Value(std::shared_ptr<ClassValue> v) : data(std::move(v)) {}

Value::Value(std::shared_ptr<InstanceValue> v) : data(std::move(v)) {}

Value::Value(std::any v) : data(std::move(v)) {}

// Implementation of type checking member functions
bool Value::is_null() const noexcept {
    return std::holds_alternative<std::monostate>(data);
}

bool Value::is_number() const noexcept {
    return std::holds_alternative<int>(data) || std::holds_alternative<float>(data) || std::holds_alternative<double>(data);
}

bool Value::is_bool() const noexcept {
    return std::holds_alternative<bool>(data);
}

bool Value::is_string() const noexcept {
    return std::holds_alternative<std::string>(data);
}

bool Value::is_array() const noexcept {
    return std::holds_alternative<std::shared_ptr<Array>>(data);
}

bool Value::is_instance() const noexcept {
    return std::holds_alternative<std::shared_ptr<InstanceValue>>(data);
}

bool Value::is_class() const noexcept {
    return std::holds_alternative<std::shared_ptr<ClassValue>>(data);
}

bool Value::is_function() const noexcept {
    return std::holds_alternative<std::shared_ptr<FunctionValue>>(data);
}

bool Value::is_int() const noexcept {
    return std::holds_alternative<int>(data);
}

bool Value::is_float() const noexcept {
    return std::holds_alternative<float>(data);
}

bool Value::is_double() const noexcept {
    return std::holds_alternative<double>(data);
}

bool Value::is_char() const noexcept {
    return std::holds_alternative<char>(data);
}

// Type conversion function implementations
float Value::as_float() const noexcept {
    if (std::holds_alternative<float>(data)) {
        return std::get<float>(data);
    }
    if (std::holds_alternative<double>(data)) {
        return static_cast<float>(std::get<double>(data));
    }
    if (std::holds_alternative<int>(data)) {
        return static_cast<float>(std::get<int>(data));
    }
    throw RuntimeError("Value is not numeric");
}

char Value::as_char() const noexcept {
    if (std::holds_alternative<char>(data)) {
        return std::get<char>(data);
    }
    throw RuntimeError("Value is not a char");
}

double Value::as_double() const noexcept {
    if (std::holds_alternative<double>(data)) {
        return std::get<double>(data);
    }
    if (std::holds_alternative<float>(data)) {
        return static_cast<double>(std::get<float>(data));
    }
    if (std::holds_alternative<int>(data)) {
        return static_cast<double>(std::get<int>(data));
    }
    if (std::holds_alternative<bool>(data)) {
        return std::get<bool>(data) ? 1.0 : 0.0;
    }
    if (std::holds_alternative<char>(data)) {
        return static_cast<double>(std::get<char>(data));
    }
    throw RuntimeError("Value is not numeric");
}

int Value::as_int() const noexcept {
    if (std::holds_alternative<int>(data)) {
        return std::get<int>(data);
    }
    if (std::holds_alternative<float>(data)) {
        return static_cast<int>(std::get<float>(data));
    }
    if (std::holds_alternative<double>(data)) {
        return static_cast<int>(std::get<double>(data));
    }
    if (std::holds_alternative<bool>(data)) {
        return std::get<bool>(data) ? 1 : 0;
    }
    if (std::holds_alternative<char>(data)) {
        return static_cast<int>(std::get<char>(data));
    }
    throw RuntimeError("Value is not convertible to int");
}

bool Value::as_bool() const noexcept {
    if (std::holds_alternative<bool>(data)) {
        return std::get<bool>(data);
    }
    if (std::holds_alternative<int>(data)) {
        return std::get<int>(data) != 0;
    }
    if (std::holds_alternative<float>(data)) {
        return std::get<float>(data) != 0.0f;
    }
    if (std::holds_alternative<double>(data)) {
        return std::get<double>(data) != 0.0;
    }
    if (std::holds_alternative<char>(data)) {
        return std::get<char>(data) != '\0';
    }
    if (std::holds_alternative<std::string>(data)) {
        return !std::get<std::string>(data).empty();
    }
    if (is_null()) {
        return false;
    }
    return true;
}

std::string Value::as_string() const noexcept {
    if (std::holds_alternative<std::string>(data)) {
        return std::get<std::string>(data);
    }

    if (std::holds_alternative<int>(data)) {
        return std::to_string(std::get<int>(data));
    }

    if (std::holds_alternative<float>(data)) {
        std::ostringstream oss;
        oss << std::get<float>(data);
        return oss.str();
    }

    if (std::holds_alternative<double>(data)) {
        std::ostringstream oss;
        oss << std::get<double>(data);
        return oss.str();
    }

    if (std::holds_alternative<bool>(data)) {
        return std::get<bool>(data) ? "true" : "false";
    }

    if (std::holds_alternative<char>(data)) {
        return std::string(1, std::get<char>(data));
    }

    if (std::holds_alternative<std::monostate>(data)) {
        return "null";
    }

    if (std::holds_alternative<std::shared_ptr<Array>>(data)) {
        auto arr = std::get<std::shared_ptr<Array>>(data);
        if (!arr) {
            return "null";
        }

        std::ostringstream oss;
        oss << "{ ";
        for (size_t i = 0; i < arr->size(); ++i) {
            if (i > 0) {
                oss << ", ";
            }
            oss << (*arr)[i].is_string();
        }
        oss << " }";
        return oss.str();
    }

    if (std::holds_alternative<std::shared_ptr<ObjectMap>>(data)) {
        auto obj = std::get<std::shared_ptr<ObjectMap>>(data);
        if (!obj) {
            return "null";
        }

        std::ostringstream oss;
        oss << "{ ";
        bool first = true;
        for (const auto& [k, v] : *obj) {
            if (!first) {
                oss << ", ";
            }
            first = false;
            oss << k << ": " << v.is_string();
        }
        oss << " }";
        return oss.str();
    }

    if (std::holds_alternative<std::shared_ptr<FunctionValue>>(data)) {
        return "<function>";
    }

    if (std::holds_alternative<std::shared_ptr<ClassValue>>(data)) {
        return "<class>";
    }

    if (std::holds_alternative<std::shared_ptr<InstanceValue>>(data)) {
        return "<instance>";
    }

    if (std::holds_alternative<std::any>(data)) {
        return "<any>";
    }

    throw RuntimeError("Value is not convertible to string");
}

std::shared_ptr<Array> Value::as_array() const noexcept {
    if (std::holds_alternative<std::shared_ptr<Array>>(data)) {
        return std::get<std::shared_ptr<Array>>(data);
    }
    throw RuntimeError("Value is not an array");
}

std::shared_ptr<InstanceValue> Value::as_instance() const noexcept {
    if (std::holds_alternative<std::shared_ptr<InstanceValue>>(data)) {
        return std::get<std::shared_ptr<InstanceValue>>(data);
    }
    throw RuntimeError("Value is not an instance");
}

std::shared_ptr<ClassValue> Value::as_class() const noexcept {
    if (std::holds_alternative<std::shared_ptr<ClassValue>>(data)) {
        return std::get<std::shared_ptr<ClassValue>>(data);
    }
    throw RuntimeError("Value is not a class");
}

std::shared_ptr<FunctionValue> Value::as_function() const noexcept {
    if (std::holds_alternative<std::shared_ptr<FunctionValue>>(data)) {
        return std::get<std::shared_ptr<FunctionValue>>(data);
    }
    throw RuntimeError("Value is not a function");
}


// Raw data getters
const Value::Variant& Value::raw() const noexcept {
    return data;
}

Value::Variant& Value::raw() {
    return data;
}

// Helpers
std::string value_type_name(const Value& v) {
    if (v.is_null()) return "null";
    if (std::holds_alternative<int>(v.raw())) return "int";
    if (std::holds_alternative<float>(v.raw())) return "float";
    if (std::holds_alternative<double>(v.raw())) return "double";
    if (std::holds_alternative<bool>(v.raw())) return "bool";
    if (std::holds_alternative<char>(v.raw())) return "char";
    if (std::holds_alternative<std::string>(v.raw())) return "string";
    if (std::holds_alternative<std::shared_ptr<Array>>(v.raw())) return "array";
    if (std::holds_alternative<std::shared_ptr<ObjectMap>>(v.raw())) return "object";
    if (std::holds_alternative<std::shared_ptr<FunctionValue>>(v.raw())) return "function";
    if (std::holds_alternative<std::shared_ptr<ClassValue>>(v.raw())) return "class";
    if (std::holds_alternative<std::shared_ptr<InstanceValue>>(v.raw())) return "instance";
    if (std::holds_alternative<std::any>(v.raw())) return "any";
    return "unknown";
}

Value apply_binary_numeric_op(const Value& a, const Value& b, const std::function<double(double, double)>& fn) {
    // Preserve ints where possible
    if (std::holds_alternative<int>(a.raw()) && std::holds_alternative<int>(b.raw())) {
        double result = fn(static_cast<double>(std::get<int>(a.raw())),
                           static_cast<double>(std::get<int>(b.raw())));

        // Only keep int if the result is mathematically integral
        if (std::floor(result) == result &&
            result >= static_cast<double>(std::numeric_limits<int>::min()) &&
            result <= static_cast<double>(std::numeric_limits<int>::max())) {
            return Value(static_cast<int>(result));
        }
        return Value(result);
    }

    if (!a.is_number() || !b.is_number()) {
        throw RuntimeError("Numeric operation requires numeric operands");
    }

    return Value(fn(a.is_double(), b.is_double()));
}

// Check if two values are equal/equivalent
bool values_equal(const Value& a, const Value& b) {
    if (a.is_null() && b.is_null()) {
        return true;
    }

    if (a.is_null() || b.is_null()) {
        return false;
    }

    if (a.is_number() && b.is_number()) {
        return std::abs(a.is_double() - b.is_double()) < 1e-9;
    }

    if (std::holds_alternative<bool>(a.raw()) && std::holds_alternative<bool>(b.raw())) {
        return std::get<bool>(a.raw()) == std::get<bool>(b.raw());
    }

    if (std::holds_alternative<char>(a.raw()) && std::holds_alternative<char>(b.raw())) {
        return std::get<char>(a.raw()) == std::get<char>(b.raw());
    }

    if (std::holds_alternative<std::string>(a.raw()) && std::holds_alternative<std::string>(b.raw())) {
        return std::get<std::string>(a.raw()) == std::get<std::string>(b.raw());
    }

    if (std::holds_alternative<std::shared_ptr<Array>>(a.raw()) &&
        std::holds_alternative<std::shared_ptr<Array>>(b.raw())) {
        return std::get<std::shared_ptr<Array>>(a.raw()) ==
               std::get<std::shared_ptr<Array>>(b.raw());
    }

    if (std::holds_alternative<std::shared_ptr<ObjectMap>>(a.raw()) &&
        std::holds_alternative<std::shared_ptr<ObjectMap>>(b.raw())) {
        return std::get<std::shared_ptr<ObjectMap>>(a.raw()) ==
               std::get<std::shared_ptr<ObjectMap>>(b.raw());
    }

    if (std::holds_alternative<std::shared_ptr<FunctionValue>>(a.raw()) &&
        std::holds_alternative<std::shared_ptr<FunctionValue>>(b.raw())) {
        return std::get<std::shared_ptr<FunctionValue>>(a.raw()) ==
               std::get<std::shared_ptr<FunctionValue>>(b.raw());
    }

    if (std::holds_alternative<std::shared_ptr<ClassValue>>(a.raw()) &&
        std::holds_alternative<std::shared_ptr<ClassValue>>(b.raw())) {
        return std::get<std::shared_ptr<ClassValue>>(a.raw()) ==
               std::get<std::shared_ptr<ClassValue>>(b.raw());
    }

    if (std::holds_alternative<std::shared_ptr<InstanceValue>>(a.raw()) &&
        std::holds_alternative<std::shared_ptr<InstanceValue>>(b.raw())) {
        return std::get<std::shared_ptr<InstanceValue>>(a.raw()) ==
               std::get<std::shared_ptr<InstanceValue>>(b.raw());
    }

    return false;
}

// Control flow signal implementations
ReturnSignal::ReturnSignal(const Value& v) : value(v) {}

ThrowSignal::ThrowSignal(const Value& v) : value(v) {}

}
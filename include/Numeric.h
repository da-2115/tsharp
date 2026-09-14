#pragma once

#include "Value.h"

namespace tsharp {

enum class NumericType { Int, Long, Float, Double };

NumericType numeric_type(const Value& value);
NumericType common_numeric_type(const Value& lhs, const Value& rhs);

Value numeric_add(const Value& lhs, const Value& rhs);
Value numeric_subtract(const Value& lhs, const Value& rhs);
Value numeric_multiply(const Value& lhs, const Value& rhs);
Value numeric_divide(const Value& lhs, const Value& rhs);
Value numeric_modulo(const Value& lhs, const Value& rhs);

int numeric_compare(const Value& lhs, const Value& rhs);

}
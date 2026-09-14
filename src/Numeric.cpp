// Numeric.cpp
// T# v2.1.1
// Dylan Armstrong, 2026

#include "Numeric.h"

#include <stdexcept>

namespace tsharp {

NumericType numeric_type(const Value& value) {
	if (value.is_int()) {
		return NumericType::Int;
	}

	if (value.is_long()) {
		return NumericType::Long;
	}

	if (value.is_float()) {
		return NumericType::Float;
	}

	if (value.is_double()) {
		return NumericType::Double;
	}

	throw RuntimeError("Value is not numeric");
}

NumericType common_numeric_type(const Value& lhs, const Value& rhs) {
	const NumericType a = numeric_type(lhs);
	const NumericType b = numeric_type(rhs);

	return static_cast<NumericType>(std::max(static_cast<int>(a), static_cast<int>(b)));
}

Value numeric_add(const Value& lhs, const Value& rhs) {
	switch (common_numeric_type(lhs, rhs)) {
		case NumericType::Int:
			return Value(lhs.as_int() + rhs.as_int());

		case NumericType::Long:
			return Value(lhs.as_long() + rhs.as_long());

		case NumericType::Float:
			return Value(lhs.as_float() + rhs.as_float());

		case NumericType::Double:
			return Value(lhs.as_double() + rhs.as_double());
	}

	throw RuntimeError("Invalid numeric addition");
}

Value numeric_subtract(const Value& lhs, const Value& rhs) {
	switch (common_numeric_type(lhs, rhs)) {
		case NumericType::Int:
			return Value(lhs.as_int() - rhs.as_int());

		case NumericType::Long:
			return Value(lhs.as_long() - rhs.as_long());

		case NumericType::Float:
			return Value(lhs.as_float() - rhs.as_float());

		case NumericType::Double:
			return Value(lhs.as_double() - rhs.as_double());
	}

	throw RuntimeError("Invalid numeric subtraction");
}

Value numeric_multiply(const Value& lhs, const Value& rhs) {
	switch (common_numeric_type(lhs, rhs)) {
		case NumericType::Int:
			return Value(lhs.as_int() * rhs.as_int());

		case NumericType::Long:
			return Value(lhs.as_long() * rhs.as_long());

		case NumericType::Float:
			return Value(lhs.as_float() * rhs.as_float());

		case NumericType::Double:
			return Value(lhs.as_double() * rhs.as_double());
	}

	throw RuntimeError("Invalid numeric multiplication");
}

Value numeric_divide(const Value& lhs, const Value& rhs) {
	if (rhs.as_double() == 0.0) {
		throw RuntimeError("Division by zero");
	}

	switch (common_numeric_type(lhs, rhs)) {
		case NumericType::Int:
			return Value(lhs.as_int() / rhs.as_int());

		case NumericType::Long:
			return Value(lhs.as_long() / rhs.as_long());

		case NumericType::Float:
			return Value(lhs.as_float() / rhs.as_float());

		case NumericType::Double:
			return Value(lhs.as_double() / rhs.as_double());
	}

	throw RuntimeError("Invalid numeric division");
}

Value numeric_modulo(const Value& lhs, const Value& rhs) {
	if (!lhs.is_int() && !lhs.is_long()) {
		throw RuntimeError("Modulo requires integer operands");
	}

	if (!rhs.is_int() && !rhs.is_long()) {
		throw RuntimeError("Modulo requires integer operands");
	}

	if (common_numeric_type(lhs, rhs) == NumericType::Long) {
		const auto divisor = rhs.as_long();

		if (divisor == 0) {
			throw RuntimeError("Modulo by zero");
		}

		return Value(lhs.as_long() % divisor);
	}

	const int divisor = rhs.as_int();

	if (divisor == 0) {
		throw RuntimeError("Modulo by zero");
	}

	return Value(lhs.as_int() % divisor);
}

int numeric_compare(const Value& lhs, const Value& rhs) {
	const NumericType type = common_numeric_type(lhs, rhs);

	switch (type) {
		case NumericType::Int:
			{
				const int a = lhs.as_int();
				const int b = rhs.as_int();
				return (a > b) - (a < b);
			}

		case NumericType::Long:
			{
				const auto a = lhs.as_long();
				const auto b = rhs.as_long();
				return (a > b) - (a < b);
			}

		case NumericType::Float:
			{
				const float a = lhs.as_float();
				const float b = rhs.as_float();
				return (a > b) - (a < b);
			}

		case NumericType::Double:
			{
				const double a = lhs.as_double();
				const double b = rhs.as_double();
				return (a > b) - (a < b);
			}
	}

	throw RuntimeError("Invalid numeric comparison");
}

}
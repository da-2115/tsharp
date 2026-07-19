// VM.cpp
// T# v2.0.0-beta1
// Dylan Armstrong, 2026

#include "VM.h"

#include "ClassValue.h"
#include "InstanceValue.h"

#include "BytecodeFunction.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace tsharp {

namespace {
struct BytecodeInstance {
	size_t class_index = 0;
	std::vector<Value> fields;
};

struct BytecodeCallable {
	enum class Kind { Method, PrimitiveCast };

	Kind kind = Kind::Method;
	size_t function_index = 0;
	Value receiver;
	std::string primitive_name;
};

static std::shared_ptr<BytecodeInstance> as_bytecode_instance(const Value& value) {
	if (!std::holds_alternative<std::any>(value.raw())) {
		return nullptr;
	}

	const auto& any_value = std::get<std::any>(value.raw());

	if (const auto* instance = std::any_cast<std::shared_ptr<BytecodeInstance>>(&any_value)) {
		return *instance;
	}

	return nullptr;
}

static Value make_bytecode_callable(size_t function_index, Value receiver) {
	return Value(std::any(BytecodeCallable{BytecodeCallable::Kind::Method, function_index, std::move(receiver), {}}));
}

static Value make_primitive_cast_callable(std::string primitive_name, Value receiver) {
	BytecodeCallable callable;
	callable.kind = BytecodeCallable::Kind::PrimitiveCast;
	callable.receiver = std::move(receiver);
	callable.primitive_name = std::move(primitive_name);
	return Value(std::any(std::move(callable)));
}

static Value primitive_cast_value(const std::string& primitive_name, const Value& receiver) {
	if (primitive_name == "to_int") {
		if (receiver.is_int()) {
			return receiver;
		}

		if (receiver.is_float()) {
			return Value(static_cast<int>(receiver.as_float()));
		}

		if (receiver.is_double()) {
			return Value(static_cast<int>(receiver.as_double()));
		}

		if (receiver.is_bool()) {
			return Value(receiver.as_bool() ? 1 : 0);
		}

		if (receiver.is_char()) {
			return Value(static_cast<int>(receiver.as_char()));
		}

		if (receiver.is_string()) {
			return Value(std::stoi(receiver.as_string()));
		}

		throw std::runtime_error("Value is not convertible to int");
	}

	if (primitive_name == "to_float") {
		if (receiver.is_float()) {
			return receiver;
		}

		if (receiver.is_int()) {
			return Value(static_cast<float>(receiver.as_int()));
		}

		if (receiver.is_double()) {
			return Value(static_cast<float>(receiver.as_double()));
		}

		if (receiver.is_bool()) {
			return Value(receiver.as_bool() ? 1.0f : 0.0f);
		}

		if (receiver.is_string()) {
			return Value(std::stof(receiver.as_string()));
		}

		throw std::runtime_error("Value is not convertible to float");
	}

	if (primitive_name == "to_double") {
		if (receiver.is_double()) {
			return receiver;
		}

		if (receiver.is_int()) {
			return Value(static_cast<double>(receiver.as_int()));
		}

		if (receiver.is_float()) {
			return Value(static_cast<double>(receiver.as_float()));
		}

		if (receiver.is_bool()) {
			return Value(receiver.as_bool() ? 1.0 : 0.0);
		}

		if (receiver.is_string()) {
			return Value(std::stod(receiver.as_string()));
		}

		throw std::runtime_error("Value is not convertible to double");
	}

	if (primitive_name == "to_string") {
		if (receiver.is_string()) {
			return receiver;
		}

		if (receiver.is_int()) {
			return Value(std::to_string(receiver.as_int()));
		}

		if (receiver.is_float()) {
			std::ostringstream stream;
			stream << receiver.as_float();

			return Value(stream.str());
		}

		if (receiver.is_double()) {
			std::ostringstream stream;
			stream << receiver.as_double();

			return Value(stream.str());
		}

		if (receiver.is_bool()) {
			return Value(receiver.as_bool() ? "true" : "false");
		}

		if (receiver.is_char()) {
			return Value(std::string(1, receiver.as_char()));
		}

		if (receiver.is_null()) {
			return Value("null");
		}

		throw std::runtime_error("Value is not convertible to string");
	}

	if (primitive_name == "to_bool") {
		if (receiver.is_bool()) {
			return receiver;
		}

		if (receiver.is_int()) {
			return Value(receiver.as_int() != 0);
		}

		if (receiver.is_float()) {
			return Value(receiver.as_float() != 0.0f);
		}

		if (receiver.is_double()) {
			return Value(receiver.as_double() != 0.0);
		}

		if (receiver.is_string()) {
			const std::string value = receiver.as_string();

			if (value == "true") {
				return Value(true);
			}

			if (value == "false") {
				return Value(false);
			}

			throw std::runtime_error("String is not convertible to bool: " + value);
		}

		throw std::runtime_error("Value is not convertible to bool");
	}

	if (primitive_name == "to_char") {
		if (receiver.is_char()) {
			return receiver;
		}

		if (receiver.is_int()) {
			return Value(static_cast<char>(receiver.as_int()));
		}

		if (receiver.is_string()) {
			const std::string value = receiver.as_string();

			if (value.size() != 1) {
				throw std::runtime_error("String must contain exactly one character");
			}

			return Value(value[0]);
		}

		throw std::runtime_error("Value is not convertible to char");
	}

	throw std::runtime_error("Unsupported primitive cast: " + primitive_name);
}

}

void VM::push(Value value) {
	get_frame().stack.push_back(std::move(value));
}

Value VM::pop() {
	auto& stack = get_frame().stack;

	if (stack.empty()) {
		throw std::runtime_error("VM stack underflow");
	}

	Value value = std::move(stack.back());

	stack.pop_back();

	return value;
}

Value& VM::peek(size_t distance) {
	auto& stack = get_frame().stack;

	if (distance >= stack.size()) {
		throw std::runtime_error("VM stack access out of bounds");
	}

	return stack[stack.size() - 1 - distance];
}

CallFrame& VM::get_frame() {
	if (frames.empty()) {
		throw std::runtime_error("No active VM call frame");
	}

	return frames.back();
}

const CallFrame& VM::get_immutable_frame() const {
	if (frames.empty()) {
		throw std::runtime_error("No active VM call frame");
	}

	return frames.back();
}

void VM::push_frame(size_t function_index, std::vector<Value> arguments, std::optional<Value> receiver) {
	if (module == nullptr) {
		throw std::runtime_error("No bytecode module loaded");
	}

	if (function_index >= module->functions.size()) {
		throw std::runtime_error("Function index out of bounds: " + std::to_string(function_index));
	}

	const BytecodeFunction& function = module->functions[function_index];

	if (arguments.size() != function.arity) {
		throw std::runtime_error("Incorrect argument count for " + function.name + ": expected " + std::to_string(function.arity) + ", received " + std::to_string(arguments.size()));
	}

	CallFrame new_frame;

	new_frame.function = &function;
	new_frame.function_index = function_index;
	new_frame.ip = 0;

	const size_t required_locals = arguments.size() + (receiver.has_value() ? 1 : 0);

	new_frame.locals.resize(std::max(function.local_count, required_locals));

	new_frame.stack.reserve(64);

	size_t argument_offset = 0;

	if (receiver.has_value()) {
		new_frame.locals[0] = std::move(*receiver);
		argument_offset = 1;
	}

	for (size_t i = 0; i < arguments.size(); i++) {
		new_frame.locals[i + argument_offset] = std::move(arguments[i]);
	}

	frames.push_back(std::move(new_frame));
}
uint8_t VM::read_byte() {
	auto& current = get_frame();

	const auto& code = current.function->chunk.get_code();

	if (current.ip >= code.size()) {
		throw std::runtime_error("Unexpected end of bytecode");
	}

	return code[current.ip++];
}

uint16_t VM::read_u16() {
	const uint16_t low = read_byte();

	const uint16_t high = read_byte();

	return static_cast<uint16_t>(low | (high << 8));
}

Value VM::run(const BytecodeModule& module) {
	this->module = &module;

	frames.clear();
	globals = module.globals;

	push_frame(module.main_function, {});

	while (!frames.empty()) {
		const auto& code = get_frame().function->chunk.get_code();
		const auto& constants = get_frame().function->chunk.get_constants();

		if (get_frame().ip >= code.size()) {
			throw std::runtime_error("Function ended without RETURN: " + get_frame().function->name);
		}

		const size_t instruction_offset = get_frame().ip;

		const uint8_t raw_opcode = read_byte();

		const auto opcode = static_cast<OpCode>(raw_opcode);

		switch (opcode) {
			case OpCode::Constant:
				{
					const uint16_t index = read_u16();

					if (index >= constants.size()) {
						throw std::runtime_error("Constant index out of bounds");
					}

					push(constants[index]);

					break;
				}

			case OpCode::Null:
				{
					push(Value());

					break;
				}

			case OpCode::True:
				{
					push(Value(true));

					break;
				}

			case OpCode::False:
				{
					push(Value(false));

					break;
				}

				// Stack operations

			case OpCode::Pop:
				{
					pop();

					break;
				}

			case OpCode::Duplicate:
				{
					push(peek());

					break;
				}

				// Local variables

			case OpCode::LoadLocal:
				{
					const uint16_t slot = read_u16();

					auto& locals = get_frame().locals;

					if (slot >= locals.size()) {
						throw std::runtime_error("Local variable slot out of bounds");
					}

					push(locals[slot]);

					break;
				}

			case OpCode::StoreLocal:
				{
					const uint16_t slot = read_u16();

					auto& locals = get_frame().locals;

					if (slot >= locals.size()) {
						locals.resize(static_cast<size_t>(slot) + 1);
					}

					locals[slot] = pop();

					break;
				}

			case OpCode::IncrementField:
				{
					const uint16_t slot = read_u16();

					Value instance_value = pop();

					if (auto instance = as_bytecode_instance(instance_value)) {
						const auto& class_info = this->module->classes[instance->class_index];

						if (slot >= class_info.fields.size()) {
							throw std::runtime_error("Field slot out of bounds");
						}

						Value& value = instance->fields[slot];

						if (value.is_int()) {
							value = Value(value.as_int() + 1);
						} else if (value.is_long()) {
							value = Value(value.as_long() + static_cast<std::int64_t>(1));
						} else if (value.is_float()) {
							value = Value(value.as_float() + 1.0f);
						} else if (value.is_double()) {
							value = Value(value.as_double() + 1.0);
						} else {
							throw std::runtime_error("Invalid field operand for ++");
						}

						break;
					}

					if (!instance_value.is_instance()) {
						throw std::runtime_error("IncrementField requires an instance");
					}

					auto instance = instance_value.as_instance();

					const auto class_it = this->module->class_lookup.find(instance->class_val->name);

					if (class_it == this->module->class_lookup.end()) {
						throw std::runtime_error("Unknown instance class: " + instance->class_val->name);
					}

					const auto& class_info = this->module->classes[class_it->second];

					if (slot >= class_info.fields.size()) {
						throw std::runtime_error("Field slot out of bounds");
					}

					const std::string& field_name = class_info.fields[slot].name;
					auto it = instance->fields.find(field_name);

					if (it == instance->fields.end()) {
						throw std::runtime_error("Undefined field: " + field_name);
					}

					Value& value = it->second;

					if (value.is_int()) {
						value = Value(value.as_int() + 1);
					} else if (value.is_long()) {
						value = Value(value.as_long() + static_cast<std::int64_t>(1));
					} else if (value.is_float()) {
						value = Value(value.as_float() + 1.0f);
					} else if (value.is_double()) {
						value = Value(value.as_double() + 1.0);
					} else {
						throw std::runtime_error("Invalid field operand for ++");
					}

					break;
				}

			case OpCode::DecrementField:
				{
					const uint16_t slot = read_u16();

					Value instance_value = pop();

					if (auto instance = as_bytecode_instance(instance_value)) {
						const auto& class_info = this->module->classes[instance->class_index];

						if (slot >= class_info.fields.size()) {
							throw std::runtime_error("Field slot out of bounds");
						}

						Value& value = instance->fields[slot];

						if (value.is_int()) {
							value = Value(value.as_int() - 1);
						} else if (value.is_long()) {
							value = Value(value.as_long() - static_cast<std::int64_t>(1));
						} else if (value.is_float()) {
							value = Value(value.as_float() - 1.0f);
						} else if (value.is_double()) {
							value = Value(value.as_double() - 1.0);
						} else {
							throw std::runtime_error("Invalid field operand for --");
						}

						break;
					}

					if (!instance_value.is_instance()) {
						throw std::runtime_error("DecrementField requires an instance");
					}

					auto instance = instance_value.as_instance();

					const auto class_it = this->module->class_lookup.find(instance->class_val->name);

					if (class_it == this->module->class_lookup.end()) {
						throw std::runtime_error("Unknown instance class: " + instance->class_val->name);
					}

					const auto& class_info = this->module->classes[class_it->second];

					if (slot >= class_info.fields.size()) {
						throw std::runtime_error("Field slot out of bounds");
					}

					const std::string& field_name = class_info.fields[slot].name;
					auto it = instance->fields.find(field_name);

					if (it == instance->fields.end()) {
						throw std::runtime_error("Undefined field: " + field_name);
					}

					Value& value = it->second;

					if (value.is_int()) {
						value = Value(value.as_int() - 1);
					} else if (value.is_long()) {
						value = Value(value.as_long() - static_cast<std::int64_t>(1));
					} else if (value.is_float()) {
						value = Value(value.as_float() - 1.0f);
					} else if (value.is_double()) {
						value = Value(value.as_double() - 1.0);
					} else {
						throw std::runtime_error("Invalid field operand for --");
					}

					break;
				}

			case OpCode::IncrementLocal:
				{
					const uint16_t slot = read_u16();

					auto& locals = get_frame().locals;

					if (slot >= locals.size()) {
						throw std::runtime_error("Local variable slot out of bounds");
					}

					Value& value = locals[slot];

					if (value.is_int()) {
						value = Value(value.as_int() + 1);
					} else if (value.is_long()) {
						value = Value(value.as_long() + static_cast<std::int64_t>(1));
					} else if (value.is_float()) {
						value = Value(value.as_float() + 1.0f);
					} else if (value.is_double()) {
						value = Value(value.as_double() + 1.0);
					} else {
						throw std::runtime_error("Invalid operand for ++");
					}

					break;
				}

			case OpCode::DecrementLocal:
				{
					const uint16_t slot = read_u16();

					auto& locals = get_frame().locals;

					if (slot >= locals.size()) {
						throw std::runtime_error("Local variable slot out of bounds");
					}

					Value& value = locals[slot];

					if (value.is_int()) {
						value = Value(value.as_int() - 1);
					} else if (value.is_long()) {
						value = Value(value.as_long() - static_cast<std::int64_t>(1));
					} else if (value.is_float()) {
						value = Value(value.as_float() - 1.0f);
					} else if (value.is_double()) {
						value = Value(value.as_double() - 1.0);
					} else {
						throw std::runtime_error("Invalid operand for --");
					}

					break;
				}

			case OpCode::AddLong:
				{
					const std::int64_t rhs = pop().as_long();
					const std::int64_t lhs = pop().as_long();

					push(Value(lhs + rhs));

					break;
				}

			case OpCode::SubtractLong:
				{
					const std::int64_t rhs = pop().as_long();
					const std::int64_t lhs = pop().as_long();

					push(Value(lhs - rhs));

					break;
				}

			case OpCode::MultiplyLong:
				{
					const std::int64_t rhs = pop().as_long();
					const std::int64_t lhs = pop().as_long();

					push(Value(lhs * rhs));

					break;
				}

			case OpCode::DivideLong:
				{
					const std::int64_t rhs = pop().as_long();
					const std::int64_t lhs = pop().as_long();

					if (rhs == 0) {
						throw std::runtime_error("Division by zero");
					}

					push(Value(lhs / rhs));

					break;
				}

			case OpCode::ModuloLong:
				{
					const std::int64_t rhs = pop().as_long();
					const std::int64_t lhs = pop().as_long();

					if (rhs == 0) {
						throw std::runtime_error("Modulo by zero");
					}

					push(Value(lhs % rhs));

					break;
				}

			case OpCode::LessLong:
				{
					const std::int64_t rhs = pop().as_long();
					const std::int64_t lhs = pop().as_long();

					push(Value(lhs < rhs));

					break;
				}

				// Globals

			case OpCode::LoadGlobal:
				{
					const uint16_t slot = read_u16();

					if (slot >= globals.size()) {
						throw std::runtime_error("Global variable slot out of bounds");
					}

					push(globals[slot]);

					break;
				}

			case OpCode::StoreGlobal:
				{
					const uint16_t slot = read_u16();

					if (slot >= globals.size()) {
						throw std::runtime_error("Global variable slot out of bounds");
					}

					globals[slot] = pop();

					break;
				}

				// Generic arithmetic

			case OpCode::Add:
				{
					const Value rhs = pop();

					const Value lhs = pop();

					push(add_values(lhs, rhs));

					break;
				}

			case OpCode::Subtract:
				{
					const Value rhs = pop();

					const Value lhs = pop();

					push(subtract_values(lhs, rhs));

					break;
				}

			case OpCode::Multiply:
				{
					const Value rhs = pop();

					const Value lhs = pop();

					push(multiply_values(lhs, rhs));

					break;
				}

			case OpCode::Divide:
				{
					const Value rhs = pop();

					const Value lhs = pop();

					push(divide_values(lhs, rhs));

					break;
				}

			case OpCode::Modulo:
				{
					const Value rhs = pop();

					const Value lhs = pop();

					push(modulo_values(lhs, rhs));

					break;
				}

				// Typed arithmetic

			case OpCode::AddInt:
				{
					const int rhs = pop().as_int();

					const int lhs = pop().as_int();

					push(Value(lhs + rhs));

					break;
				}

			case OpCode::AddFloat:
				{
					const float rhs = pop().as_float();

					const float lhs = pop().as_float();

					push(Value(lhs + rhs));

					break;
				}

			case OpCode::AddDouble:
				{
					const double rhs = pop().as_double();

					const double lhs = pop().as_double();

					push(Value(lhs + rhs));

					break;
				}

			case OpCode::AddString:
				{
					const Value rhs = pop();

					const Value lhs = pop();

					push(Value(value_to_string(lhs) + value_to_string(rhs)));

					break;
				}

			case OpCode::SubtractInt:
				{
					const int rhs = pop().as_int();

					const int lhs = pop().as_int();

					push(Value(lhs - rhs));

					break;
				}

			case OpCode::SubtractFloat:
				{
					const float rhs = pop().as_float();

					const float lhs = pop().as_float();

					push(Value(lhs - rhs));

					break;
				}

			case OpCode::SubtractDouble:
				{
					const double rhs = pop().as_double();

					const double lhs = pop().as_double();

					push(Value(lhs - rhs));

					break;
				}

			case OpCode::MultiplyInt:
				{
					const int rhs = pop().as_int();

					const int lhs = pop().as_int();

					push(Value(lhs * rhs));

					break;
				}

			case OpCode::MultiplyFloat:
				{
					const float rhs = pop().as_float();

					const float lhs = pop().as_float();

					push(Value(lhs * rhs));

					break;
				}

			case OpCode::MultiplyDouble:
				{
					const double rhs = pop().as_double();

					const double lhs = pop().as_double();

					push(Value(lhs * rhs));

					break;
				}

			case OpCode::DivideInt:
				{
					const int rhs = pop().as_int();

					const int lhs = pop().as_int();

					if (rhs == 0) {
						throw std::runtime_error("Division by zero");
					}

					push(Value(lhs / rhs));

					break;
				}

			case OpCode::DivideFloat:
				{
					const float rhs = pop().as_float();

					const float lhs = pop().as_float();

					if (rhs == 0.0f) {
						throw std::runtime_error("Division by zero");
					}

					push(Value(lhs / rhs));

					break;
				}

			case OpCode::DivideDouble:
				{
					const double rhs = pop().as_double();

					const double lhs = pop().as_double();

					if (rhs == 0.0) {
						throw std::runtime_error("Division by zero");
					}

					push(Value(lhs / rhs));

					break;
				}

			case OpCode::ModuloInt:
				{
					const int rhs = pop().as_int();

					const int lhs = pop().as_int();

					if (rhs == 0) {
						throw std::runtime_error("Modulo by zero");
					}

					push(Value(lhs % rhs));

					break;
				}

				// Unary negation

			case OpCode::Negate:
				{
					const Value value = pop();

					if (value.is_int()) {
						push(Value(-value.as_int()));
					} else if (value.is_float()) {
						push(Value(-value.as_float()));
					} else if (value.is_double()) {
						push(Value(-value.as_double()));
					} else {
						throw std::runtime_error("Invalid operand for unary -");
					}

					break;
				}

			case OpCode::NegateInt:
				{
					push(Value(-pop().as_int()));

					break;
				}

			case OpCode::NegateFloat:
				{
					push(Value(-pop().as_float()));

					break;
				}

			case OpCode::NegateDouble:
				{
					push(Value(-pop().as_double()));

					break;
				}

				// Equality

			case OpCode::Equal:
				{
					const Value rhs = pop();

					const Value lhs = pop();

					push(Value(values_equal(lhs, rhs)));

					break;
				}

			case OpCode::NotEqual:
				{
					const Value rhs = pop();

					const Value lhs = pop();

					push(Value(!values_equal(lhs, rhs)));

					break;
				}

				// Generic comparison

			case OpCode::Less:
				{
					const Value rhs = pop();

					const Value lhs = pop();

					if (!lhs.is_number() || !rhs.is_number()) {
						throw std::runtime_error("Invalid operands for <");
					}

					push(Value(lhs.as_double() < rhs.as_double()));

					break;
				}

			case OpCode::LessEqual:
				{
					const Value rhs = pop();

					const Value lhs = pop();

					if (!lhs.is_number() || !rhs.is_number()) {
						throw std::runtime_error("Invalid operands for <=");
					}

					push(Value(lhs.as_double() <= rhs.as_double()));

					break;
				}

			case OpCode::Greater:
				{
					const Value rhs = pop();

					const Value lhs = pop();

					if (!lhs.is_number() || !rhs.is_number()) {
						throw std::runtime_error("Invalid operands for >");
					}

					push(Value(lhs.as_double() > rhs.as_double()));

					break;
				}

			case OpCode::GreaterEqual:
				{
					const Value rhs = pop();

					const Value lhs = pop();

					if (!lhs.is_number() || !rhs.is_number()) {
						throw std::runtime_error("Invalid operands for >=");
					}

					push(Value(lhs.as_double() >= rhs.as_double()));

					break;
				}

				// Typed integer comparison

			case OpCode::LessInt:
				{
					const int rhs = pop().as_int();

					const int lhs = pop().as_int();

					push(Value(lhs < rhs));

					break;
				}

			case OpCode::LessEqualInt:
				{
					const int rhs = pop().as_int();

					const int lhs = pop().as_int();

					push(Value(lhs <= rhs));

					break;
				}

			case OpCode::GreaterInt:
				{
					const int rhs = pop().as_int();

					const int lhs = pop().as_int();

					push(Value(lhs > rhs));

					break;
				}

			case OpCode::GreaterEqualInt:
				{
					const int rhs = pop().as_int();

					const int lhs = pop().as_int();

					push(Value(lhs >= rhs));

					break;
				}

				// Logical operations

			case OpCode::LogicalAnd:
				{
					const Value rhs = pop();

					const Value lhs = pop();

					push(Value(is_truthy(lhs) && is_truthy(rhs)));

					break;
				}

			case OpCode::LogicalOr:
				{
					const Value rhs = pop();

					const Value lhs = pop();

					push(Value(is_truthy(lhs) || is_truthy(rhs)));

					break;
				}

			case OpCode::LogicalNot:
				{
					const Value value = pop();

					push(Value(!is_truthy(value)));

					break;
				}

				// Arrays

			case OpCode::NewArray:
				{
					const int size = pop().as_int();

					if (size < 0) {
						throw std::runtime_error("Array size cannot be negative");
					}

					auto array = std::make_shared<Array>();

					array->resize(static_cast<size_t>(size));

					push(Value(array));

					break;
				}

			case OpCode::ArrayLiteral:
				{
					const uint16_t count = read_u16();

					auto array = std::make_shared<Array>();

					array->resize(count);

					for (size_t i = count; i > 0; i--) {
						(*array)[i - 1] = pop();
					}

					push(Value(array));

					break;
				}

			case OpCode::LoadIndex:
				{
					const int index = pop().as_int();

					const Value target = pop();

					if (!target.is_array()) {
						throw std::runtime_error("Cannot index a non-array value");
					}

					auto array = target.as_array();

					if (index < 0 || static_cast<size_t>(index) >= array->size()) {
						throw std::runtime_error("Array index out of bounds: index=" + std::to_string(index) + ", size=" + std::to_string(array->size()));
					}

					push((*array)[static_cast<size_t>(index)]);

					break;
				}

			case OpCode::StoreIndex:
				{
					Value value = pop();

					const int index = pop().as_int();

					Value target = pop();

					if (!target.is_array()) {
						throw std::runtime_error("Cannot index a non-array value");
					}

					auto array = target.as_array();

					if (index < 0 || static_cast<size_t>(index) >= array->size()) {
						throw std::runtime_error("Array index out of bounds");
					}

					(*array)[static_cast<size_t>(index)] = std::move(value);

					// Assignment is still an expression.
					push((*array)[static_cast<size_t>(index)]);

					break;
				}

			case OpCode::ArrayLength:
				{
					Value target = pop();

					if (!target.is_array()) {
						throw std::runtime_error("ArrayLength requires an array");
					}

					push(Value(static_cast<int>(target.as_array()->size())));

					break;
				}

				// Control flow

			case OpCode::Jump:
				{
					const uint16_t offset = read_u16();

					get_frame().ip += offset;

					break;
				}

			case OpCode::JumpIfFalse:
				{
					const uint16_t offset = read_u16();

					const Value condition = pop();

					if (!is_truthy(condition)) {
						get_frame().ip += offset;
					}

					break;
				}

			case OpCode::JumpIfTrue:
				{
					const uint16_t offset = read_u16();

					const Value condition = pop();

					if (is_truthy(condition)) {
						get_frame().ip += offset;
					}

					break;
				}

			case OpCode::Loop:
				{
					const uint16_t offset = read_u16();

					if (offset > get_frame().ip) {
						throw std::runtime_error("Invalid loop offset");
					}

					get_frame().ip -= offset;

					break;
				}

				// Function calls

			case OpCode::Call:
				{
					const uint16_t function_index = read_u16();

					const uint16_t argument_count = read_u16();

					std::vector<Value> arguments(argument_count);

					for (size_t i = argument_count; i > 0; i--) {
						arguments[i - 1] = pop();
					}

					push_frame(function_index, std::move(arguments));

					break;
				}

			case OpCode::CallNative:
				{
					const uint16_t native_index = read_u16();

					const uint16_t argument_count = read_u16();

					std::vector<Value> arguments(argument_count);

					for (size_t i = argument_count; i > 0; i--) {
						arguments[i - 1] = pop();
					}

					push(call_native(native_index, arguments));

					break;
				}

			case OpCode::Return:
				{
					Value result = get_frame().stack.empty() ? Value() : pop();

					frames.pop_back();

					if (frames.empty()) {
						return result;
					}

					push(std::move(result));

					break;
				}

				// Direct print opcodes

			case OpCode::Print:
				{
					const Value value = pop();

					std::cout << value_to_string(value);

					// Every expression leaves one result.
					push(Value());

					break;
				}

			case OpCode::PrintLine:
				{
					const Value value = pop();

					std::cout << value_to_string(value) << '\n';

					push(Value());

					break;
				}

				// Halt

			case OpCode::Halt:
				{
					Value result = get_frame().stack.empty() ? Value() : pop();

					frames.clear();

					return result;
				}

			case OpCode::NewObject:
				{
					const uint16_t class_index = read_u16();
					const uint16_t argument_count = read_u16();

					if (!this->module) {
						throw std::runtime_error("No bytecode this->module loaded");
					}

					if (class_index >= this->module->classes.size()) {
						throw std::runtime_error("Class index out of bounds");
					}

					std::vector<Value> arguments(argument_count);

					for (size_t i = argument_count; i > 0; i--) {
						arguments[i - 1] = pop();
					}

					auto instance = std::make_shared<BytecodeInstance>();
					instance->class_index = class_index;
					instance->fields.resize(this->module->classes[class_index].fields.size());

					const auto& class_info = this->module->classes[class_index];
					size_t constructor_index = std::numeric_limits<size_t>::max();

					for (size_t candidate : class_info.constructors) {
						if (candidate < this->module->functions.size() && this->module->functions[candidate].arity == argument_count) {
							constructor_index = candidate;
							break;
						}
					}

					Value instance_value{std::any(instance)};

					if (constructor_index == std::numeric_limits<size_t>::max()) {
						if (argument_count != 0 && !class_info.constructors.empty()) {
							throw std::runtime_error("No matching constructor for " + class_info.name + " with " + std::to_string(argument_count) + " arguments");
						}

						push(std::move(instance_value));
						break;
					}

					push_frame(constructor_index, std::move(arguments), std::optional<Value>(std::move(instance_value)));

					break;
				}
			case OpCode::LoadThis:
				{
					push(get_frame().locals.at(0));

					break;
				}

			case OpCode::LoadField:
				{
					const uint16_t slot = read_u16();

					Value target = pop();

					if (auto instance = as_bytecode_instance(target)) {
						const auto& class_info = this->module->classes[instance->class_index];

						if (slot >= class_info.fields.size()) {
							throw std::runtime_error("Field slot out of bounds");
						}

						push(instance->fields[slot]);

						break;
					}

					if (target.is_instance()) {
						auto instance = target.as_instance();
						if (!instance || !instance->class_val) {
							throw std::runtime_error("LoadField requires an instance");
						}

						const auto class_it = this->module->class_lookup.find(instance->class_val->name);

						if (class_it == this->module->class_lookup.end()) {
							throw std::runtime_error("Unknown instance class: " + instance->class_val->name);
						}

						const auto& class_info = this->module->classes[class_it->second];
						if (slot >= class_info.fields.size()) {
							throw std::runtime_error("Field slot out of bounds");
						}

						const std::string& field_name = class_info.fields[slot].name;
						auto field_it = instance->fields.find(field_name);

						if (field_it == instance->fields.end()) {
							throw std::runtime_error("Undefined field: " + field_name);
						}

						push(field_it->second);
						break;
					}

					throw std::runtime_error("LoadField requires an instance");
				}
			case OpCode::StoreField:
				{
					const uint16_t slot = read_u16();
					Value value = pop();
					Value target = pop();

					if (auto instance = as_bytecode_instance(target)) {
						const auto& class_info = this->module->classes[instance->class_index];

						if (slot >= class_info.fields.size()) {
							throw std::runtime_error("Field slot out of bounds");
						}

						instance->fields[slot] = value;
						push(value);
						break;
					}

					if (target.is_instance()) {
						auto instance = target.as_instance();
						if (!instance || !instance->class_val) {
							throw std::runtime_error("StoreField requires an instance");
						}

						const auto class_it = this->module->class_lookup.find(instance->class_val->name);

						if (class_it == this->module->class_lookup.end()) {
							throw std::runtime_error("Unknown instance class: " + instance->class_val->name);
						}

						const auto& class_info = this->module->classes[class_it->second];

						if (slot >= class_info.fields.size()) {
							throw std::runtime_error("Field slot out of bounds");
						}

						instance->fields[class_info.fields[slot].name] = value;
						push(value);
						break;
					}

					throw std::runtime_error("StoreField requires an instance");
				}

			case OpCode::StoreFieldDynamic:
				{
					const uint16_t constant_index = read_u16();

					const auto& member_name_value = constants.at(constant_index);
					const std::string field_name = member_name_value.as_string();

					Value value = pop();
					Value target = pop();

					if (auto instance = as_bytecode_instance(target)) {
						const auto& class_info = this->module->classes[instance->class_index];

						auto field_it = class_info.field_slots.find(field_name);

						if (field_it == class_info.field_slots.end()) {
							throw std::runtime_error("Undefined field: " + field_name);
						}

						instance->fields[field_it->second] = value;
						push(value);
						break;
					}

					if (target.is_instance()) {
						auto instance = target.as_instance();
						if (!instance || !instance->class_val) {
							throw std::runtime_error("StoreFieldDynamic requires an instance");
						}

						instance->fields[field_name] = value;
						push(value);
						break;
					}

					throw std::runtime_error("StoreFieldDynamic requires an instance");
				}

			case OpCode::LoadMember:
			case OpCode::GetProperty:
				{
					const uint16_t constant_index = read_u16();

					const auto& member_name_value = constants.at(constant_index);
					const std::string member_name = member_name_value.as_string();

					Value target = pop();

					if (auto instance = as_bytecode_instance(target)) {
						const auto& class_info = this->module->classes[instance->class_index];

						auto field_it = class_info.field_slots.find(member_name);

						if (field_it != class_info.field_slots.end()) {
							push(instance->fields[field_it->second]);
							break;
						}

						auto property_it = class_info.properties.find(member_name);

						if (property_it != class_info.properties.end()) {
							push_frame(property_it->second, {}, std::optional<Value>(target));
							break;
						}

						auto method_it = class_info.methods.find(member_name);
						if (method_it != class_info.methods.end()) {
							push(make_bytecode_callable(method_it->second, target));
							break;
						}

						throw std::runtime_error("Unknown member: " + member_name + " on bytecode class " + class_info.name + " (fields=" + std::to_string(class_info.fields.size()) +
												 ", methods=" + std::to_string(class_info.methods.size()) + ", properties=" + std::to_string(class_info.properties.size()) + ")");
					}

					if (target.is_int() || target.is_float() || target.is_double() || target.is_bool() || target.is_char() || target.is_string()) {
						if (member_name == "to_int" || member_name == "to_float" || member_name == "to_double" || member_name == "to_string" || member_name == "to_bool" || member_name == "to_char") {
							push(make_primitive_cast_callable(member_name, target));
							break;
						}
					}

					throw std::runtime_error("Unknown member: " + member_name);
				}

			case OpCode::AddLocalInt:
				{
					const uint16_t destination = read_u16();

					const uint16_t source = read_u16();

					auto& locals = get_frame().locals;

					if (destination >= locals.size() || source >= locals.size()) {
						throw std::runtime_error("AddLocalInt local slot out of bounds");
					}

					if (!locals[destination].is_int() || !locals[source].is_int()) {
						throw std::runtime_error("AddLocalInt requires int locals");
					}

					locals[destination] = Value(locals[destination].as_int() + locals[source].as_int());

					break;
				}

			case OpCode::AddLocalLong:
				{
					const uint16_t destination = read_u16();

					const uint16_t source = read_u16();

					auto& locals = get_frame().locals;

					if (destination >= locals.size() || source >= locals.size()) {
						throw std::runtime_error("AddLocalLong local slot out of bounds");
					}

					const bool destination_is_integer = locals[destination].is_int() || locals[destination].is_long();

					const bool source_is_integer = locals[source].is_int() || locals[source].is_long();

					if (!destination_is_integer || !source_is_integer) {
						throw std::runtime_error("AddLocalLong requires integer locals");
					}

					locals[destination] = Value(locals[destination].as_long() + locals[source].as_long());

					break;
				}

			case OpCode::LessLocalIntConstant:
				{
					const uint16_t local_slot = read_u16();

					const uint16_t constant_index = read_u16();

					auto& locals = get_frame().locals;
					const auto& constants = get_immutable_frame().function->chunk.get_constants();

					if (local_slot >= locals.size() || constant_index >= constants.size()) {
						throw std::runtime_error("LessLocalIntConstant operand out of bounds");
					}

					push(Value(locals[local_slot].as_int() < constants[constant_index].as_int()));

					break;
				}
			case OpCode::LessLocalLongConstant:
				{
					const uint16_t local_slot = read_u16();

					const uint16_t constant_index = read_u16();

					auto& locals = get_frame().locals;
					const auto& constants = get_immutable_frame().function->chunk.get_constants();

					if (local_slot >= locals.size() || constant_index >= constants.size()) {
						throw std::runtime_error("LessLocalLongConstant operand out of bounds");
					}

					push(Value(locals[local_slot].as_long() < constants[constant_index].as_long()));

					break;
				}
			case OpCode::CallMethod:
				{
					const uint16_t function_index = read_u16();

					const uint16_t argument_count = read_u16();

					if (function_index >= this->module->functions.size()) {
						throw std::runtime_error("CallMethod function index out of bounds");
					}

					std::vector<Value> arguments(argument_count);

					for (size_t i = argument_count; i > 0; i--) {
						arguments[i - 1] = pop();
					}

					Value receiver = pop();

					if (!as_bytecode_instance(receiver)) {
						throw std::runtime_error("CallMethod requires a bytecode instance");
					}

					push_frame(function_index, std::move(arguments), std::optional<Value>(std::move(receiver)));

					break;
				}
			case OpCode::CallVirtual:
			case OpCode::CallConstructor:
			case OpCode::LoadEnumValue:
			case OpCode::Cast:
			case OpCode::ToInt:
			case OpCode::ToFloat:
			case OpCode::ToDouble:
			case OpCode::ToString:
			case OpCode::ToBool:
			case OpCode::ToChar:
			case OpCode::TypeOf:
			case OpCode::Switch:
				{
					throw std::runtime_error("Opcode not implemented yet at offset " + std::to_string(instruction_offset) + ": " + std::to_string(static_cast<int>(static_cast<uint8_t>(opcode))));
				}

			case OpCode::CallValue:
				{
					const uint16_t argument_count = read_u16();

					std::vector<Value> arguments(argument_count);

					for (size_t i = argument_count; i > 0; i--) {
						arguments[i - 1] = pop();
					}

					Value callee = pop();

					if (std::holds_alternative<std::any>(callee.raw())) {
						const auto& any_value = std::get<std::any>(callee.raw());

						if (const auto* callable = std::any_cast<BytecodeCallable>(&any_value)) {
							if (callable->kind == BytecodeCallable::Kind::PrimitiveCast) {
								if (!arguments.empty()) {
									throw std::runtime_error("Primitive cast methods do not take arguments");
								}

								push(primitive_cast_value(callable->primitive_name, callable->receiver));
								break;
							}

							push_frame(callable->function_index, std::move(arguments), std::optional<Value>(callable->receiver));

							break;
						}
					}

					if (callee.is_function()) {
						throw std::runtime_error("Bytecode function values are not yet callable through CallValue");
					}

					throw std::runtime_error("Value is not callable");
				}

			case OpCode::PushExceptionHandler:
				{
					const uint16_t offset = read_u16();

					get_frame().exception_handlers.push_back(ExceptionHandler{.catch_ip = get_frame().ip + offset, .stack_size = get_frame().stack.size()});

					break;
				}

			case OpCode::PopExceptionHandler:
				{
					if (get_frame().exception_handlers.empty()) {
						throw std::runtime_error("PopExceptionHandler with no active handler");
					}

					get_frame().exception_handlers.pop_back();

					break;
				}

			case OpCode::Throw:
				{
					Value exception_value = pop();
					bool handled = false;

					while (!frames.empty()) {
						CallFrame& current = get_frame();

						if (!current.exception_handlers.empty()) {
							auto handler = current.exception_handlers.back();
							current.exception_handlers.pop_back();
							current.stack.resize(handler.stack_size);
							current.stack.push_back(exception_value);
							current.ip = handler.catch_ip;
							handled = true;
							break;
						}

						frames.pop_back();
					}

					if (!handled) {
						throw std::runtime_error("Unhandled exception: " + value_to_string(exception_value));
					}

					break;
				}

			default:
				{
					throw std::runtime_error("Unknown bytecode instruction at offset " + std::to_string(instruction_offset) + ": " + std::to_string(static_cast<int>(static_cast<uint8_t>(opcode))));
				}
		}
	}
	return Value();
}

Value VM::call_native(size_t native_index, const std::vector<Value>& arguments) {
	if (!this->module) {
		throw std::runtime_error("No bytecode this->module loaded");
	}

	if (native_index >= this->module->natives.size()) {
		throw std::runtime_error("Native function index out of bounds");
	}

	const auto& native = this->module->natives[native_index];

	const std::string& name = native.name;

	if (name == "file_read_all_text") {
		if (arguments.size() != 1) {
			throw std::runtime_error("file_read_all_text expects 1 argument");
		}

		const std::string path = arguments[0].as_string();

		std::ifstream file(path);

		if (!file.is_open()) {
			throw std::runtime_error("Could not open file: " + path);
		}

		std::ostringstream buffer;
		buffer << file.rdbuf();

		return Value(buffer.str());
	}

	if (name == "file_write_all_text") {
		if (arguments.size() != 2) {
			throw std::runtime_error("file_write_all_text expects 2 arguments");
		}

		const std::string path = arguments[0].as_string();
		const std::string text = arguments[1].as_string();

		std::ofstream file(path);

		if (!file.is_open()) {
			throw std::runtime_error("Could not open file for writing: " + path);
		}

		file << text;

		return Value();
	}

	if (name == "file_append_all_text") {
		if (arguments.size() != 2) {
			throw std::runtime_error("file_append_all_text expects 2 arguments");
		}

		const std::string path = arguments[0].as_string();
		const std::string text = arguments[1].as_string();

		std::ofstream file(path, std::ios::app);

		if (!file.is_open()) {
			throw std::runtime_error("Could not open file for appending: " + path);
		}

		file << text;

		return Value();
	}

	if (name == "file_exists") {
		if (arguments.size() != 1) {
			throw std::runtime_error("file_exists expects 1 argument");
		}

		return Value(std::filesystem::exists(arguments[0].as_string()));
	}

	if (name == "file_delete") {
		if (arguments.size() != 1) {
			throw std::runtime_error("file_delete expects 1 argument");
		}

		std::filesystem::remove(arguments[0].as_string());

		return Value();
	}

	if (name == "file_copy") {
		if (arguments.size() != 2) {
			throw std::runtime_error("file_copy expects 2 arguments");
		}

		std::filesystem::copy_file(arguments[0].as_string(), arguments[1].as_string(), std::filesystem::copy_options::overwrite_existing);

		return Value();
	}

	if (name == "file_move") {
		if (arguments.size() != 2) {
			throw std::runtime_error("file_move expects 2 arguments");
		}

		std::filesystem::rename(arguments[0].as_string(), arguments[1].as_string());

		return Value();
	}

	if (name == "print") {
		if (arguments.size() != 1) {
			throw std::runtime_error("print expects 1 argument");
		}

		std::cout << value_to_string(arguments[0]);

		return Value();
	}

	if (name == "println") {
		if (arguments.size() != 1) {
			throw std::runtime_error("println expects 1 argument");
		}

		std::cout << value_to_string(arguments[0]) << '\n';

		return Value();
	}

	if (name == "sqrt") {
		if (arguments.size() != 1) {
			throw std::runtime_error("sqrt expects 1 argument");
		}

		return Value(std::sqrt(arguments[0].as_double()));
	}

	if (name == "abs") {
		if (arguments.size() != 1) {
			throw std::runtime_error("abs expects 1 argument");
		}

		if (arguments[0].is_int()) {
			return Value(std::abs(arguments[0].as_int()));
		}

		return Value(std::abs(arguments[0].as_double()));
	}

	if (name == "cube_root") {
		if (arguments.size() != 1) {
			throw std::runtime_error("cube_root expects 1 argument");
		}

		return Value(std::cbrt(arguments[0].as_double()));
	}

	if (name == "exp") {
		if (arguments.size() != 1) {
			throw std::runtime_error("exp expects 1 argument");
		}

		return Value(std::exp(arguments[0].as_double()));
	}

	if (name == "log") {
		if (arguments.size() != 1) {
			throw std::runtime_error("log expects 1 argument");
		}

		return Value(std::log(arguments[0].as_double()));
	}

	if (name == "sin") {
		return Value(std::sin(arguments[0].as_double()));
	}

	if (name == "cos") {
		return Value(std::cos(arguments[0].as_double()));
	}

	if (name == "tan") {
		return Value(std::tan(arguments[0].as_double()));
	}

	if (name == "pow") {
		if (arguments.size() != 2) {
			throw std::runtime_error("pow expects 2 arguments");
		}

		return Value(std::pow(arguments[0].as_double(), arguments[1].as_double()));
	}

	if (name == "x_root") {
		if (arguments.size() != 2) {
			throw std::runtime_error("x_root expects 2 arguments");
		}

		return Value(std::pow(arguments[0].as_double(), 1.0 / arguments[1].as_double()));
	}

	if (name == "floor") {
		return Value(std::floor(arguments[0].as_double()));
	}

	if (name == "ceil") {
		return Value(std::ceil(arguments[0].as_double()));
	}

	if (name == "round") {
		return Value(std::round(arguments[0].as_double()));
	}

	if (name == "min") {
		if (arguments.size() != 2) {
			throw std::runtime_error("min expects 2 arguments");
		}

		if (arguments[0].is_int() && arguments[1].is_int()) {
			return Value(std::min(arguments[0].as_int(), arguments[1].as_int()));
		}

		return Value(std::min(arguments[0].as_double(), arguments[1].as_double()));
	}

	if (name == "max") {
		if (arguments.size() != 2) {
			throw std::runtime_error("max expects 2 arguments");
		}

		if (arguments[0].is_int() && arguments[1].is_int()) {
			return Value(std::max(arguments[0].as_int(), arguments[1].as_int()));
		}

		return Value(std::max(arguments[0].as_double(), arguments[1].as_double()));
	}

	if (name == "factorial") {
		if (arguments.size() != 1) {
			throw std::runtime_error("factorial expects 1 argument");
		}

		const int n = arguments[0].as_int();

		if (n < 0) {
			throw std::runtime_error("factorial requires a non-negative integer");
		}

		int result = 1;

		for (int i = 2; i <= n; i++) {
			result *= i;
		}

		return Value(result);
	}

	if (name == "size") {
		if (arguments.size() != 1) {
			throw std::runtime_error("size expects 1 argument");
		}

		if (arguments[0].is_array()) {
			return Value(static_cast<int>(arguments[0].as_array()->size()));
		}

		if (arguments[0].is_string()) {
			return Value(static_cast<int>(arguments[0].as_string().size()));
		}

		throw std::runtime_error("size requires an array or string");
	}

	if (name == "typeof") {
		if (arguments.size() != 1) {
			throw std::runtime_error("typeof expects 1 argument");
		}

		const Value& value = arguments[0];

		if (value.is_null()) {
			return Value("null");
		}

		if (value.is_long()) {
			return Value("long");
		}

		if (value.is_int()) {
			return Value("int");
		}

		if (value.is_float()) {
			return Value("float");
		}

		if (value.is_double()) {
			return Value("double");
		}

		if (value.is_bool()) {
			return Value("bool");
		}

		if (value.is_char()) {
			return Value("char");
		}

		if (value.is_string()) {
			return Value("string");
		}

		if (value.is_array()) {
			return Value("array");
		}

		if (value.is_instance()) {
			return Value("instance");
		}

		if (value.is_class()) {
			return Value("class");
		}

		if (value.is_function()) {
			return Value("function");
		}

		return Value("unknown");
	}

	if (name == "push") {
		if (arguments.size() != 2) {
			throw std::runtime_error("push expects 2 arguments");
		}

		if (!arguments[0].is_array()) {
			throw std::runtime_error("push expects first argument to be an array");
		}

		arguments[0].as_array()->push_back(arguments[1]);

		return Value();
	}

	if (name == "pop") {
		if (arguments.size() != 1) {
			throw std::runtime_error("pop expects 1 argument");
		}

		if (!arguments[0].is_array()) {
			throw std::runtime_error("pop expects argument to be an array");
		}

		auto array = arguments[0].as_array();

		if (array->empty()) {
			throw std::runtime_error("pop on empty array");
		}

		Value last = array->back();
		array->pop_back();

		return last;
	}

	if (name == "sort") {
		if (arguments.size() != 1) {
			throw std::runtime_error("sort expects 1 argument");
		}

		if (!arguments[0].is_array()) {
			throw std::runtime_error("sort expects argument to be an array");
		}

		auto array = arguments[0].as_array();

		auto less_than = [this](const Value& lhs, const Value& rhs) {
			if (lhs.is_number() && rhs.is_number()) {
				return lhs.as_double() < rhs.as_double();
			}

			if (lhs.is_string() && rhs.is_string()) {
				return lhs.as_string() < rhs.as_string();
			}

			return value_to_string(lhs) < value_to_string(rhs);
		};

		for (size_t i = 0; i < array->size(); i++) {
			size_t smallest = i;

			for (size_t j = i + 1; j < array->size(); j++) {
				if (less_than((*array)[j], (*array)[smallest])) {
					smallest = j;
				}
			}

			if (smallest != i) {
				std::swap((*array)[i], (*array)[smallest]);
			}
		}

		return Value();
	}

	if (name == "address") {
		if (arguments.size() != 1) {
			throw std::runtime_error("address expects 1 argument");
		}

		const Value& value = arguments[0];
		const void* address = nullptr;

		if (value.is_array()) {
			address = static_cast<const void*>(value.as_array().get());
		} else if (auto instance = as_bytecode_instance(value)) {
			address = static_cast<const void*>(instance.get());
		} else if (value.is_instance()) {
			address = static_cast<const void*>(value.as_instance().get());
		} else if (value.is_class()) {
			address = static_cast<const void*>(value.as_class().get());
		} else if (value.is_function()) {
			address = static_cast<const void*>(value.as_function().get());
		} else {
			throw std::runtime_error("address requires an array, instance, class or function");
		}

		std::ostringstream stream;
		stream << address;

		return Value(stream.str());
	}

	throw std::runtime_error("Native function not implemented: " + name);
}

bool VM::values_equal(const Value& lhs, const Value& rhs) const {
	if (lhs.is_null() && rhs.is_null()) {
		return true;
	}

	if (lhs.is_null() || rhs.is_null()) {
		return false;
	}

	if (lhs.is_number() && rhs.is_number()) {
		return (lhs.as_double() == rhs.as_double());
	}

	if (lhs.is_bool() && rhs.is_bool()) {
		return (lhs.as_bool() == rhs.as_bool());
	}

	if (lhs.is_char() && rhs.is_char()) {
		return (lhs.as_char() == rhs.as_char());
	}

	if (lhs.is_string() && rhs.is_string()) {
		return (lhs.as_string() == rhs.as_string());
	}

	if (lhs.is_array() && rhs.is_array()) {
		return (lhs.as_array().get() == rhs.as_array().get());
	}

	if (lhs.is_instance() && rhs.is_instance()) {
		return (lhs.as_instance().get() == rhs.as_instance().get());
	}

	return false;
}

bool VM::is_truthy(const Value& value) const {
	if (value.is_null()) {
		return false;
	}

	if (value.is_bool()) {
		return value.as_bool();
	}

	if (value.is_int()) {
		return (value.as_int() != 0);
	}

	if (value.is_float()) {
		return (value.as_float() != 0.0f);
	}

	if (value.is_double()) {
		return (value.as_double() != 0.0);
	}

	if (value.is_string()) {
		return (!value.as_string().empty());
	}

	if (value.is_long()) {
		return value.as_long() != 0;
	}

	return true;
}

std::string VM::value_to_string(const Value& value) const {
	if (value.is_null()) {
		return "null";
	}

	if (value.is_int()) {
		return std::to_string(value.as_int());
	}

	if (value.is_long()) {
		return std::to_string(value.as_long());
	}

	if (value.is_float()) {
		std::ostringstream stream;

		stream << value.as_float();

		return stream.str();
	}

	if (value.is_double()) {
		std::ostringstream stream;

		stream << value.as_double();

		return stream.str();
	}

	if (value.is_bool()) {
		return (value.as_bool() ? "true" : "false");
	}

	if (value.is_char()) {
		return std::string(1, value.as_char());
	}

	if (value.is_string()) {
		return value.as_string();
	}

	if (value.is_array()) {
		std::string result = "[";

		const auto array = value.as_array();

		for (size_t i = 0; i < array->size(); i++) {
			if (i > 0) {
				result += ", ";
			}

			result += value_to_string((*array)[i]);
		}

		result += "]";

		return result;
	}

	if (value.is_instance()) {
		return "<instance>";
	}

	return "<value>";
}

Value VM::add_values(const Value& lhs, const Value& rhs) const {
	if (lhs.is_long() || rhs.is_long()) {
		if ((lhs.is_int() || lhs.is_long()) && (rhs.is_int() || rhs.is_long())) {
			return Value(lhs.as_long() + rhs.as_long());
		}
	}
	if (lhs.is_string() || rhs.is_string()) {
		return Value(value_to_string(lhs) + value_to_string(rhs));
	}

	if (lhs.is_int() && rhs.is_int()) {
		return Value(lhs.as_int() + rhs.as_int());
	}

	if (lhs.is_double() || rhs.is_double()) {
		return Value(lhs.as_double() + rhs.as_double());
	}

	if (lhs.is_float() || rhs.is_float()) {
		return Value(static_cast<float>(lhs.as_double() + rhs.as_double()));
	}

	throw std::runtime_error("Invalid operands for +");
}

Value VM::subtract_values(const Value& lhs, const Value& rhs) const {
	if (!lhs.is_number() || !rhs.is_number()) {
		throw std::runtime_error("Invalid operands for -");
	}

	if (lhs.is_int() && rhs.is_int()) {
		return Value(lhs.as_int() - rhs.as_int());
	}

	if (lhs.is_double() || rhs.is_double()) {
		return Value(lhs.as_double() - rhs.as_double());
	}

	return Value(static_cast<float>(lhs.as_double() - rhs.as_double()));
}

Value VM::multiply_values(const Value& lhs, const Value& rhs) const {
	if (!lhs.is_number() || !rhs.is_number()) {
		throw std::runtime_error("Invalid operands for *");
	}

	if (lhs.is_int() && rhs.is_int()) {
		return Value(lhs.as_int() * rhs.as_int());
	}

	if (lhs.is_double() || rhs.is_double()) {
		return Value(lhs.as_double() * rhs.as_double());
	}

	return Value(static_cast<float>(lhs.as_double() * rhs.as_double()));
}
Value VM::divide_values(const Value& lhs, const Value& rhs) const {
	if (!lhs.is_number() || !rhs.is_number()) {
		throw std::runtime_error("Invalid operands for /");
	}

	if (rhs.as_double() == 0.0) {
		throw std::runtime_error("Division by zero");
	}

	if (lhs.is_int() && rhs.is_int()) {
		return Value(lhs.as_int() / rhs.as_int());
	}

	if (lhs.is_double() || rhs.is_double()) {
		return Value(lhs.as_double() / rhs.as_double());
	}

	return Value(static_cast<float>(lhs.as_double() / rhs.as_double()));
}
Value VM::modulo_values(const Value& lhs, const Value& rhs) const {
	if (!lhs.is_int() || !rhs.is_int()) {
		throw std::runtime_error("Modulo requires integer operands");
	}

	if (rhs.as_int() == 0) {
		throw std::runtime_error("Modulo by zero");
	}

	return Value(lhs.as_int() % rhs.as_int());
}
}
// VM.h
// T# v2.0.0-beta1
// Dylan Armstrong, 2026

#pragma once

#include "Value.h"

#include "BytecodeFunction.h"
#include "BytecodeModule.h"
#include "OpCode.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <vector>

namespace tsharp {

// For exceptions
struct ExceptionHandler {
	size_t catch_ip = 0;
	size_t stack_size = 0;
};

// Call frame
struct CallFrame {
	const BytecodeFunction* function = nullptr;

	size_t function_index = 0;
	size_t ip = 0;

	std::vector<Value> locals;

	std::vector<Value> stack;

	std::vector<ExceptionHandler> exception_handlers;
};

// The T# virtual machine
class VM {
  public:
	Value run(const BytecodeModule& module);

  private:
	const BytecodeModule* module = nullptr;

	std::vector<CallFrame> frames;
	std::vector<Value> globals;

	CallFrame& get_frame();

	const CallFrame& get_immutable_frame() const;

	void push(Value value);

	Value pop();

	Value& peek(size_t distance = 0);

	uint8_t read_byte();

	uint16_t read_u16();

	void push_frame(size_t function_index, std::vector<Value> arguments, std::optional<Value> receiver = std::nullopt);

	// Value return_from_frame(Value result);

	Value call_native(size_t native_index, const std::vector<Value>& arguments);

	bool values_equal(const Value& lhs, const Value& rhs) const;

	bool is_truthy(const Value& value) const;

	std::string value_to_string(const Value& value) const;

	Value add_values(const Value& lhs, const Value& rhs) const;

	Value subtract_values(const Value& lhs, const Value& rhs) const;

	Value multiply_values(const Value& lhs, const Value& rhs) const;

	Value divide_values(const Value& lhs, const Value& rhs) const;

	Value modulo_values(const Value& lhs, const Value& rhs) const;
};

}
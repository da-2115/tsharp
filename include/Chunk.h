// Chunk.h
// T# v2.0.0-beta1
// Dylan Armstrong, 2026

#pragma once

#include "OpCode.h"
#include "Value.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace tsharp {

// Chunk class definition
class Chunk {
  private:
	std::vector<uint8_t> code;
	std::vector<Value> constants;

  public:
	// Write byte
	void write_byte(uint8_t byte);

	// Write opcode
	void write_opcode(OpCode opcode);

	// Add a constant
	size_t add_constant(Value value);

	// Get code, constants - returns a constant vector
	const std::vector<uint8_t>& get_code() const;
	const std::vector<Value>& get_constants() const;

	// Return size of code vector
	size_t code_size() const {
		return code.size();
	}

	// Return a mutable version of the code
	std::vector<uint8_t>& mutable_code() {
		return code;
	}
};

}
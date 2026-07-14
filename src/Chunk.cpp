// Chunk.cpp
// T# v2.0.0-beta1
// Dylan Armstrong, 2026

#include "Chunk.h"

#include <utility>

namespace tsharp {

void Chunk::write_byte(std::uint8_t byte) {
	code.push_back(byte);
}

void Chunk::write_opcode(OpCode opcode) {
	code.push_back(static_cast<std::uint8_t>(opcode));
}

std::size_t Chunk::add_constant(Value value) {
	constants.push_back(std::move(value));

	return constants.size() - 1;
}

const std::vector<std::uint8_t>& Chunk::get_code() const {
	return code;
}

const std::vector<Value>& Chunk::get_constants() const {
	return constants;
}

}
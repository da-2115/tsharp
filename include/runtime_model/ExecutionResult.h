// ExecutionResult.h
// T# v1.0.0-betarc (v1.0.0 Beta Release Candidate/Final Beta)
// Dylan Armstrong, 2026

#pragma once

#include "ControlFlow.h"
#include "Value.h"

namespace tsharp {
class Interpreter;

// Execution result
// Stores a value, and the control flow of that value
struct ExecutionResult {
	Value value;
	ControlFlow flow = ControlFlow::NORMAL;
};
}
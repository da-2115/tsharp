// ExecutionResult.h

// Dylan Armstrong, 2026

#pragma once

#include "ControlFlow.h"
#include "Value.h"

namespace tsharp {
;

// Execution result
// Stores a value, and the control flow of that value
struct ExecutionResult {
	Value value;
	ControlFlow flow = ControlFlow::NORMAL;
};
}
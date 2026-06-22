// ExecutionResult.h
// Dylan Armstrong, 2026

#pragma once

#include "ControlFlow.h"
#include "Value.h"

namespace tsharp {
class Interpreter;

struct ExecutionResult {
    Value value;
    ControlFlow flow = ControlFlow::NORMAL;
};
}
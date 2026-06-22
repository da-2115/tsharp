// MemberInfo.h
// Dylan Armstrong, 2026

#pragma once

#include <memory>

#include "PropertyValue.h"
#include "FieldInfo.h"
#include "FunctionValue.h"

namespace tsharp {
// Forward declaration for interpreter class - which implements the visitor pattern logic
class Interpreter;

struct MemberInfo {
    enum Type { FIELD, METHOD, PROPERTY } type;
    std::shared_ptr<FunctionValue> method;  // When type == METHOD
    PropertyValue property;                 // When type == PROPERTY
    FieldInfo field_meta;                   // Metadata for all types
};

}
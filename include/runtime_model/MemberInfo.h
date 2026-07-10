// MemberInfo.h
// T# v1.0.0-betarc (v1.0.0 Beta Release Candidate/Final Beta)
// Dylan Armstrong, 2026

#pragma once

#include <memory>

#include "FieldInfo.h"
#include "FunctionValue.h"
#include "PropertyValue.h"

namespace tsharp {
// Forward declaration for interpreter class - which implements the visitor pattern logic
class Interpreter;

// MemberInfo struct
// Stores information about member variables, methods, properties
struct MemberInfo {
	enum Type { FIELD,
				METHOD,
				PROPERTY } type;
	std::shared_ptr<FunctionValue> method; // When type == METHOD
	PropertyValue property;				   // When type == PROPERTY
	FieldInfo field_meta;				   // Metadata for all types
};

}
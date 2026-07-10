// FieldInfo.h
// T# v1.0.0-betarc (v1.0.0 Beta Release Candidate/Final Beta)
// Dylan Armstrong, 2026

#pragma once

#include <string>

namespace tsharp {
class Interpreter;

// Field info struct for tracking field access modifiers
struct FieldInfo {
	std::string type_name;

	// Is it static? private? protected? public? - which access modifier is it?
	bool is_static = false;
	bool is_private = false;
	bool is_protected = false;
	bool is_public = true;
};

}
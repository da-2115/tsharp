// ParameterInfo.h
// T# v1.0.0-betarc (v1.0.0 Beta Release Candidate/Final Beta)
// Dylan Armstrong, 2026

#pragma once

#include <string>

namespace tsharp {
class Interpreter;

// Parameter info struct
struct ParameterInfo {
	std::string type_name;
	std::string name;
};

}
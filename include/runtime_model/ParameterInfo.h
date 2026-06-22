// ParameterInfo.h
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
// tsharp_types.h
// Dylan Armstrong, 2026

#pragma once

#include <string>

// Types in T#
enum class tsharp_types {
    INT,
    STRING,
    FLOAT,
    VOID
};

// Convert type from enum to string in listener
std::string type_to_string(tsharp_types type);
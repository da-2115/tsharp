// tsharp_types.cpp
// Dylan Armstrong, 2026

#include "tsharp_types.h"

// Convert type enum to string
std::string type_to_string(tsharp_types type) {
    switch (type) {
        case tsharp_types::INT:
            return "int";
        case tsharp_types::STRING:
            return "string";
        case tsharp_types::FLOAT:
            return "float";
        case tsharp_types::VOID:
            return "void";
        default:
            return "any";
    }
}
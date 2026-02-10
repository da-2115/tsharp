// tsharp_value.cpp
// Dylan Armstrong, 2026

#include "tsharp_value.h"

// Four different constructors -> for different types
tsharp_value::tsharp_value(int v) : value(v), type("int") {
}

tsharp_value::tsharp_value(float v) : value(v), type("float") {
}

tsharp_value::tsharp_value(const std::string& v) : value(v), type("string") {
}

tsharp_value::tsharp_value(bool v) : value(v), type("bool") {
}

std::string tsharp_value::get_type() const {
    return type;
}
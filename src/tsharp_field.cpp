// tsharp_field.cpp
// Dylan Armstrong, 2026

#include "tsharp_field.h"

tsharp_field::tsharp_field(const tsharp_value& value, const bool is_private) 
    : value(value), is_private(is_private) {
}

const tsharp_value& tsharp_field::get_value() const {
    return value;
}

void tsharp_field::set_value(const tsharp_value&& new_value) {
    value = std::move(new_value);
}

bool tsharp_field::get_is_private() const {
    return is_private;
}
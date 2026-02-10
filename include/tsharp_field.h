// tsharp_field.h
// Dylan Armstrong, 2026

#pragma once

#include <string>

#include "tsharp_value.h"

class tsharp_field {
private:
    tsharp_value value; // Use tsharp_value here, not std::string
    bool is_private;

public:
    tsharp_field(const tsharp_value& value, const bool is_private);

    const tsharp_value& get_value() const;
    void set_value(const tsharp_value&& new_value);
    bool get_is_private() const;
};
// tsharp_constructor.h
// Dylan Armstrong, 2026

#pragma once

#include <memory>
#include <vector>

#include "tsharp_function.h"
#include "tsharp_value.h"
#include "tsharp_field.h"

// Inherits from tsharp_function due to similiar behaviour
class tsharp_constructor : public tsharp_function {
public:
    tsharp_constructor(const std::string& type, const std::vector<tsharp_argument>& arguments, const std::string& return_value);

    void execute(const std::vector<tsharp_value>& args, std::vector<tsharp_field>& fields);
};
#pragma once

#include <string>

enum class tsharp_types {
    INT,
    STRING
};

std::string type_to_string(tsharp_types type);
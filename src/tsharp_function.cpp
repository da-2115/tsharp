// tsharp_function.cpp
// Dylan Armstrong, 2026

#include "tsharp_function.h"

tsharp_function::tsharp_function(const std::string& type, const std::vector<tsharp_argument>& arguments, const std::string& return_value) 
    : type(type), arguments(arguments), return_value(return_value) {   
}

const tsharp_argument& tsharp_function::get_argument_by_index(size_t index) const {
    return arguments.at(index);
}

const std::vector<tsharp_argument>& tsharp_function::get_arguments() const {
    return arguments;
}

const std::string tsharp_function::get_type() const {
    return type;
}

const std::string tsharp_function::get_ret_value() const {
    return return_value;
}

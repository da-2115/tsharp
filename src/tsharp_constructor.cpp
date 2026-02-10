// tsharp_constructor.cpp
// Dylan Armstrong, 2026

#include "tsharp_constructor.h"

#include "tsharp_class.h"

tsharp_constructor::tsharp_constructor(const std::string& type, const std::vector<tsharp_argument>& arguments, const std::string& return_value) 
    : tsharp_function(type, arguments, return_value) {
}

void tsharp_constructor::execute(const std::vector<tsharp_value>& args, tsharp_class& obj) {
    obj.set_fields(args);
}
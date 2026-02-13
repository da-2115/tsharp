// tsharp_constructor.cpp
// Dylan Armstrong, 2026

#include "tsharp_constructor.h"

#include "tsharp_class.h"

tsharp_constructor::tsharp_constructor(const std::string& type, const std::vector<tsharp_argument>& arguments, const std::string& return_value) 
    : tsharp_function(type, arguments, return_value) {
}

void tsharp_constructor::execute(const std::vector<tsharp_value>& args, tsharp_class& obj) {
    // Match arguments to fields using the constructor's formal parameter names
    for (size_t i = 0; i < get_arguments().size() && i < args.size(); i++) {
        const tsharp_argument& param = get_argument_by_index(i);
        obj.set_field_by_name(param.var_name, args[i]);
    }
}

bool tsharp_constructor::operator==(const tsharp_constructor &other) const {
    return get_type() == other.get_type() && get_ret_value() == other.get_ret_value();
}

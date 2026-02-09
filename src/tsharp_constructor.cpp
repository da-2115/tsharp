#include "tsharp_constructor.h"

tsharp_constructor::tsharp_constructor(const std::string& type, const std::vector<tsharp_argument>& arguments, const std::string& return_value) 
    : tsharp_function(type, arguments, return_value) {
}

void tsharp_constructor::execute(const std::vector<tsharp_value>& args, std::vector<tsharp_field>& fields) {
    // Set fields
    if (fields.empty()) {
        return;
    }

    for (size_t i = 0; i < fields.size() && i < args.size(); ++i) {
        fields.at(i).set_value(args[i]);
    }
}

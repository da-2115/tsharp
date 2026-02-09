#include "tsharp_class.h"

#include <algorithm>

tsharp_class::tsharp_class()
    : fields{}, constructors{}, methods{} {
}

tsharp_class::tsharp_class(const tsharp_class& other) 
    : fields(other.fields), constructors(other.constructors), methods(other.methods) {
}

const tsharp_field tsharp_class::get_field(const std::string& index) const {
    return fields.at(index);
}

std::vector<tsharp_field> tsharp_class::get_fields() const {
    std::vector<tsharp_field> vec;

    for (std::map<std::string, tsharp_field>::const_iterator it = fields.begin(); it != fields.end(); ++it) {
        vec.push_back(it->second);
    }

    return vec;
}

void tsharp_class::add_field(const std::string& name, const tsharp_value& value, const bool is_private) {
    fields.emplace(name, tsharp_field(value, is_private));
}

void tsharp_class::set_field(const std::string& name, const tsharp_value& new_value) {
    fields.at(name).set_value(new_value);
}

size_t tsharp_class::field_count() const {
    return fields.size();
}

const tsharp_constructor& tsharp_class::get_constructor(const std::string& index) const {
    return constructors.at(index);
}

void tsharp_class::add_constructor(const std::string& name, const std::string& type, const std::vector<tsharp_argument>& arguments, const std::string& return_value) {
    constructors.emplace(name, tsharp_constructor(type, arguments, return_value));
}

const tsharp_function& tsharp_class::get_method(const std::string& index) const {
    return methods.at(index);
}

void tsharp_class::add_method(const std::string& name, const std::string& type, const std::vector<tsharp_argument>& arguments, const std::string& return_value) {
    methods.emplace(name, tsharp_function(type, arguments, return_value));
}

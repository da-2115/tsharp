// tsharp_class.cpp
// Dylan Armstrong, 2026

#include "tsharp_class.h"

#include <algorithm>

tsharp_class::tsharp_class()
    : fields{}, constructors{}, methods{} {
}

// Copy constructor implementation
tsharp_class::tsharp_class(const tsharp_class& other) 
    : fields(other.fields), constructors(other.constructors), methods(other.methods) {
}

tsharp_field tsharp_class::get_field(const std::string& index) const {
    return fields.at(index);
}

// Get ALL fields as std::vector with tsharp_field objects as the items in the vector
std::vector<tsharp_field> tsharp_class::get_fields() const {
    std::vector<tsharp_field> vec;

    // Iterate over the map of fields
    for (std::map<std::string, tsharp_field>::const_iterator it = fields.begin(); it != fields.end(); ++it) {
        vec.push_back(it->second);
    }

    return vec;
}

void tsharp_class::add_field(const std::string& name, const tsharp_value&& value, const bool is_private) {
    fields.emplace(name, tsharp_field(std::move(value), is_private));
}

void tsharp_class::set_field(const std::string& name, const tsharp_value&& new_value) {
    fields.at(name).set_value(std::move(new_value));
}

void tsharp_class::set_fields(const std::vector<tsharp_value>& values) {
    size_t i = 0;
    
    for (auto& [name, field] : fields) {
        if (i < values.size()) {
            field.set_value(std::move(values[i++]));
        }
    }
}

// Return number of fields
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

// Get size -> dereference this object
size_t tsharp_class::get_size() const {
    return sizeof(*this);
}

// tsharp_class.h
// Dylan Armstrong, 2026

#pragma once

#include <map>

#include "tsharp_field.h"
#include "tsharp_constructor.h"
#include "tsharp_function.h"

class tsharp_class {
private:
    std::map<std::string, tsharp_field> fields;
    std::map<std::string, tsharp_constructor> constructors;
    std::map<std::string, tsharp_function> methods;

public:
    tsharp_class();
    
    tsharp_class(const tsharp_class& other);

    const tsharp_field get_field(const std::string& index) const;
    std::vector<tsharp_field> get_fields() const;

    void add_field(const std::string& name, const tsharp_value& value, const bool is_private);
    void set_field(const std::string& name, const tsharp_value& new_value);
    size_t field_count() const;
    
    const tsharp_constructor& get_constructor(const std::string& index) const;
    void add_constructor(const std::string& name, const std::string& type, const std::vector<tsharp_argument>& arguments, const std::string& return_value);

    const tsharp_function& get_method(const std::string& index) const;
    void add_method(const std::string& name, const std::string& type, const std::vector<tsharp_argument>& arguments, const std::string& return_value);
};
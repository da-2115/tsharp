// tsharp_class.cpp
// Dylan Armstrong, 2026

#include "tsharp_class.h"
#include "tsharp_value.h"

#include <algorithm>
#include <iostream>
#include <variant>

tsharp_class::tsharp_class() : fields{}, constructors{}, methods{} {
}

// Copy constructor implementation
tsharp_class::tsharp_class(const tsharp_class& other) : fields(other.fields), constructors(other.constructors), methods(other.methods) {
}

const tsharp_field& tsharp_class::get_field(const std::string& index) const {
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

void tsharp_class::set_field(const std::string& name, const tsharp_value& new_value) {
	fields.at(name).set_value(new_value);
}

// Helper function to convert a value to match a target type
tsharp_value tsharp_class::convert_to_type(const tsharp_value& target_template, const tsharp_value& input_value) {
	auto target_variant = target_template.get_value();
	auto input_variant = input_value.get_value();

	if (std::holds_alternative<int>(target_variant)) {
		if (std::holds_alternative<int>(input_variant))
			return input_value;
		if (std::holds_alternative<float>(input_variant))
			return tsharp_value((int)std::get<float>(input_variant));
		if (std::holds_alternative<std::string>(input_variant))
			return tsharp_value(std::stoi(std::get<std::string>(input_variant)));
		if (std::holds_alternative<bool>(input_variant))
			return tsharp_value((int)std::get<bool>(input_variant));
	}

	else if (std::holds_alternative<float>(target_variant)) {
		if (std::holds_alternative<float>(input_variant))
			return input_value;
		if (std::holds_alternative<int>(input_variant))
			return tsharp_value((float)std::get<int>(input_variant));
		if (std::holds_alternative<std::string>(input_variant)) {
			std::string float_str = std::get<std::string>(input_variant);
			// Strip trailing 'f' or 'd' suffix if present
			if (!float_str.empty() && (float_str.back() == 'f' || float_str.back() == 'd')) {
				float_str.pop_back();
			}
			return tsharp_value(std::stof(float_str));
		}
		if (std::holds_alternative<bool>(input_variant))
			return tsharp_value((float)std::get<bool>(input_variant));
	}

	else if (std::holds_alternative<std::string>(target_variant)) {
		if (std::holds_alternative<std::string>(input_variant))
			return input_value;
		if (std::holds_alternative<int>(input_variant))
			return tsharp_value(std::to_string(std::get<int>(input_variant)));
		if (std::holds_alternative<float>(input_variant))
			return tsharp_value(std::to_string(std::get<float>(input_variant)));
		if (std::holds_alternative<bool>(input_variant))
			return tsharp_value(std::to_string(std::get<bool>(input_variant)));
	}

	else if (std::holds_alternative<bool>(target_variant)) {
		if (std::holds_alternative<bool>(input_variant))
			return input_value;
		if (std::holds_alternative<int>(input_variant))
			return tsharp_value((bool)std::get<int>(input_variant));
		if (std::holds_alternative<float>(input_variant))
			return tsharp_value((bool)std::get<float>(input_variant));
		if (std::holds_alternative<std::string>(input_variant)) {
			bool bool_value = (std::get<std::string>(input_variant) == "true" || std::get<std::string>(input_variant) == "1");
			return tsharp_value(bool_value);
		}
	}

	return input_value;
}

void tsharp_class::set_fields(const std::vector<tsharp_value>& values) {
	size_t i = 0;

	for (auto& [name, field] : fields) {
		if (i < values.size()) {
			field.set_value(convert_to_type(field.get_value(), values[i]));
			i++;
		}
	}
}

void tsharp_class::set_field_by_name(const std::string& name, const tsharp_value& value) {
	// Try to find field with exact name
	if (fields.find(name) != fields.end()) {
		fields.at(name).set_value(convert_to_type(fields.at(name).get_value(), value));
		return;
	}

	// Try with underscore prefix if not found
	std::string underscore_name = "_" + name;
	if (fields.find(underscore_name) != fields.end()) {
		fields.at(underscore_name).set_value(convert_to_type(fields.at(underscore_name).get_value(), value));
		return;
	}
}

// Return number of fields
size_t tsharp_class::field_count() const {
	return fields.size();
}

const tsharp_constructor& tsharp_class::get_constructor(const std::string& index) const {
	return constructors.at(index);
}

void tsharp_class::add_constructor(const std::string& name, const std::string& type, const std::vector<tsharp_argument>& arguments,
                                   const std::string& return_value) {
	constructors.emplace(name, tsharp_constructor(type, arguments, return_value));
}

const std::vector<tsharp_constructor> tsharp_class::get_constructors() const {
	std::vector<tsharp_constructor> vec;

	// Iterate over the map of fields
	for (std::map<std::string, tsharp_constructor>::const_iterator it = constructors.begin(); it != constructors.end(); ++it) {
		vec.push_back(it->second);
	}

	return vec;
}

const tsharp_function& tsharp_class::get_method(const std::string& index) const {
	return methods.at(index);
}

void tsharp_class::add_method(const std::string& name, const std::string& type, const std::vector<tsharp_argument>& arguments,
                              const std::string& return_value) {
	methods.emplace(name, tsharp_function(type, arguments, return_value));
}

// Get size -> dereference this object
size_t tsharp_class::get_size() const {
	return sizeof(*this);
}

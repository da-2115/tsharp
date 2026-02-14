// tsharp_function.h
// Dylan Armstrong, 2026

#pragma once

#include "tsharp_types.h"

#include <vector>

// tsharp_argument struct
struct tsharp_argument {
	std::string type;
	std::string var_name;
};

class tsharp_function {
private:
	std::string type;
	std::vector<tsharp_argument> arguments;
	std::string return_value;

public:
	tsharp_function(const std::string& type, const std::vector<tsharp_argument>& arguments, const std::string& return_value);

	const tsharp_argument& get_argument_by_index(size_t index) const;
	const std::vector<tsharp_argument>& get_arguments() const;
	const std::string get_type() const;
	const std::string get_ret_value() const;

	// Function return template function
	template <class T> 
    T func_return(const std::string& value) const {
		if (type == INT_TYPE) {
			return std::stoi(value);
		}
	}
};
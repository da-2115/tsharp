// tsharp_value.h
// Dylan Armstrong, 2026

#pragma once

#include <any>
#include <memory>
#include <string>
#include <variant>

class tsharp_value {
private:
	std::variant<int, float, std::string, bool, std::any> value;

public:
	tsharp_value(int v);
	tsharp_value(float v);
	tsharp_value(const std::string& v);
	tsharp_value(bool v);
    
	template<typename T>
	tsharp_value(const T& v) : value(std::any(v)) {}

	std::variant<int, float, std::string, bool, std::any> get_value() const;
};
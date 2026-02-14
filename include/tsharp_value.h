// tsharp_value.h
// Dylan Armstrong, 2026

#pragma once

#include <string>
#include <variant>

class tsharp_value {
private:
	std::variant<int, float, std::string, bool> value;

public:
	tsharp_value(int v);
	tsharp_value(float v);
	tsharp_value(const std::string& v);
	tsharp_value(bool v);

	std::variant<int, float, std::string, bool> get_value() const;
};
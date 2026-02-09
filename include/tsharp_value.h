// tsharp_value.h
// Dylan Armstrong, 2026

#pragma once

#include <string>
#include <variant>

class tsharp_value {
private:
    std::variant<int, float, std::string, bool> value;
    std::string type;

public:
    tsharp_value(int v);
    tsharp_value(float v);
    tsharp_value(const std::string& v);
    tsharp_value(bool v);

    template<typename T>
    T get() const { return value; }

    std::string get_type() const;
};
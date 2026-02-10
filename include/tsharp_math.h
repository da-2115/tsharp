// tsharp_math.h
// Dylan Armstrong, 2026

#pragma once

#include <cmath>
#include <iostream>

// tsharp math functions namespace
namespace tsharp_math {
    // Exponent function, takes two types as argument list
    template<class T, class U>
    T exp(const T& value, const U& exponent) {
        return std::pow(value, exponent);
    }
    
    // Square root function, raise value to the power of 1/2
    template<class T>
    T sqrt(const T& value) {
        return tsharp_math::exp<T, float>(value, 0.5);
    }

    // Pi
    template<class T>
    constexpr T pi = T(3.1415926);

    // Absolute value function
    template<class T>
    T abs(const T& value) {
        return value;
    }
}
// tsharp_math.h
// Dylan Armstrong, 2026

#pragma once

#include <cmath>
#include <iostream>

// tsharp math functions namespace
namespace tsharp_math {
    // Square root function, raise value to the power of 1/2
    template<class T>
    T sqrt(const T& value) {
        return tsharp_math::exp<T, float>(value, 0.5);
    }
}

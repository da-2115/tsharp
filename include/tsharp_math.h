#pragma once

#include <cmath>
#include <iostream>

namespace tsharp_math {
    template<class T>
    T abs(const T& value) {
        return +(0 - value);
    }

    template<class T, class U>
    T exp(const T& value, const U& exponent) {
        return std::pow(value, exponent);
    }

    template<class T>
    T sqrt(const T& value) {
        return tsharp_math::exp<T, float>(value, 0.5);
    }

    template<class T>
    T cos(const T& value) {
        return std::cos(value * 180 / 3.1415926);
    }

    template<class T>
    T sin(const T& value) {
        return std::sin(value);
    }

    template<class T>
    T tan(const T& value) {
        return std::tan(value);
    }
}

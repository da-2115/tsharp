#pragma once

#include <cmath>

template<class T>
T abs(const T& value) {
    return +(0 - value);
}

template<class T>
T sqrt(const T& value) {
    return exp(value, 0.5);
}

template<class T>
T exp(const T& value, const T& exponent) {
    return pow(value, exponent);
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
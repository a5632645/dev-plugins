#pragma once
#include <numbers>
#include <cmath>

namespace qwqdsp::fastmath {
/**
 * @param x [-pi/2, pi/2]
 */
template<class T>
static constexpr T Sin(T x) noexcept {
    constexpr T pi = std::numbers::pi_v<T>;
    constexpr T B = 4 / pi;
    constexpr T C = -4 / (pi * pi);
    T const y = B * x + C * x * std::abs(x);
    constexpr T P = static_cast<T>(0.225);
    return y + P * (y * std::abs(y) - y);
}
}
#pragma once
#include <numbers>
#include <cmath>

namespace qwqdsp::fastmath {
/**
 * @brief 作为振荡器使用约有-70dB左右的伪影
 * @param x [-pi, pi]
 * @ref https://www.cnblogs.com/sun11086/archive/2009/03/20/1417944.html
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
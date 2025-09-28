#pragma once
#include <cmath>
#include <numbers>
#include <complex>

namespace qwqdsp::polymath {
/**
 * @param x [0, pi]
 * .\lolremez.exe --degree 4 --range 1e-50:pi*pi "sin(sqrt(x))/(sqrt(x)*x)-1/x" "1/(x*sqrt(x))"
 */
template<class T>
static inline constexpr T SinPi(T x) noexcept {
    T const x2 = x * x;
    T u = static_cast<T>(-2.0549870296337988e-8);
    u = u * x2 + static_cast<T>(2.7051369051832768e-6);
    u = u * x2 + static_cast<T>(-1.9814449939486949e-4);
    u = u * x2 + static_cast<T>(8.3326852207856376e-3);
    T const f = u * x2 + static_cast<T>(-1.6666612015728217e-1);
    return x * x2 * f + x;
}

/**
 * @param x [0, pi]
 * .\lolremez --degree 4 --range 1e-50:pi*pi "(cos(sqrt(x))-1)/x" "1/x"
 */
template<class T>
static inline constexpr T CosPi(T x) noexcept {
    T const x2 = x * x;
    T u = static_cast<T>(-2.2068375498948933e-7);
    u = u * x2 + static_cast<T>(2.4228489043422979e-5);
    u = u * x2 + static_cast<T>(-1.3861265327344013e-3);
    u = u * x2 + static_cast<T>(4.1660818319866521e-2);
    T const f = u * x2 + static_cast<T>(-4.9999596805416058e-1);
    return 1 + x2 * f;
}

template<class T>
static inline constexpr T Cos(T x) noexcept {
    return CosPi(std::abs(x));
}

/**
 * @param x [0, 1]
 * .\lolremez --degree 4 --range 0:1 "pow(2,x)"
 */
template<class T>
static inline constexpr T Exp2Half(T x) noexcept {
    T u = static_cast<T>(1.3697664475809267e-2);
    u = u * x + static_cast<T>(5.1690358205939469e-2);
    u = u * x + static_cast<T>(2.4163844572498163e-1);
    u = u * x + static_cast<T>(6.9296612266139567e-1);
    return u * x + static_cast<T>(1.000003704465937);
}

template<class T>
static inline constexpr T Exp2(T x) noexcept {
    int const i = static_cast<int>(x);
    return static_cast<T>(1 << i) * Exp2Half(x);
}

/**
 * @brief 最快，最不精确
 * @param x [-pi, pi]
 * @ref https://www.cnblogs.com/sun11086/archive/2009/03/20/1417944.html
 */
template<class T>
static inline constexpr T SinParabola(T x) noexcept {
    constexpr T pi = std::numbers::pi_v<T>;
    constexpr T B = 4 / pi;
    constexpr T C = -4 / (pi * pi);
    T const y = B * x + C * x * std::abs(x);
    constexpr T P = static_cast<T>(0.225);
    return y + P * (y * std::abs(y) - y);
}

// based on https://stackoverflow.com/questions/79601848/sine-approximation-did-i-beat-remez

/**
 * @brief 最慢，最精确
 * @param x [-pi/2, pi/2]
 */
template<class T>
static inline constexpr T SinRemez(T x) {
    T t = x * x;
    T p =       static_cast<T>(-2.38889015e-8); // -0x1.9a6880p-26
    p = p * t + static_cast<T>(2.75253933e-6); //  0x1.717088p-19
    p = p * t - static_cast<T>(1.98408685e-4); // -0x1.a017dap-13
    p = p * t + static_cast<T>(8.33333377e-3); //  0x1.111112p-7
    p = p * t - static_cast<T>(1.66666672e-1); // -0x1.555556p-3
    t = t * x;
    p = p * t + x;
    return p;
}

/**
 * @brief 第二快，第二精确
 * @param x [-pi/2, pi/2]
 */
template<class T>
static constexpr inline T SinRemezRat(T x) {
    T s = x * x;
    T q = static_cast<T>(-2.91886134e-3); // -0x1.7e94bcp-9
    T p = static_cast<T>(-1.64994095e-2); // -0x1.0e538ap-6
    T t = s * x;
    q = q * s - static_cast<T>(2.00993851e-1); // -0x1.9ba2aap-3
    p = p * s;
    q = q * s - static_cast<T>(6.00000238e+0); // -0x1.80000ap+2
    p = p * t + t;
    return (p / q) + x;
}

// based on https://github.com/chenzt2020/foc_learning/blob/main/3.fast_sin/fast_sin.h
namespace internal {
// lolremez --float --degree 5 --range "1e-50:pi*pi"
// "(sin(sqrt(x))-sqrt(x))/(x*sqrt(x))" "1/(x*sqrt(x))"
// Estimated max error: 1.455468e-9
static inline constexpr float f1(float x) {
    float u = 1.3528548e-10f;
    u = u * x + -2.4703144e-08f;
    u = u * x + 2.7532926e-06f;
    u = u * x + -0.00019840381f;
    u = u * x + 0.0083333179f;
    return u * x + -0.16666666f;
}
// lolremez --float --degree 5 --range "1e-50:pi*pi" "(cos(sqrt(x))-1)/x"
// "1/x"
// Estimated max error: 1.1846383e-8
static inline constexpr float f2(float x) {
    float u = 1.7290616e-09f;
    u = u * x + -2.7093486e-07f;
    u = u * x + 2.4771643e-05f;
    u = u * x + -0.0013887906f;
    u = u * x + 0.041666519f;
    return u * x + -0.49999991f;
}
}

// 只有-2pi < x < 2pi时才会比std::sin快一点点（指不到1.00倍）
static inline constexpr float FastSin(float x) {
    // si = (int)(x / pi)
    int si = (int)(x * 0.31830988f);
    x = x - (float)si * std::numbers::pi_v<float>;
    if (si & 1) {
        x = x > 0.0f ? x - std::numbers::pi_v<float> : x + std::numbers::pi_v<float>;
    }
    return x + x * x * x * internal::f1(x * x);
}

// 只有-2pi < x < 2pi时才会比std::sin快一点点（指不到1.00倍）
static inline constexpr float FastCos(float x) {
    // si = (int)(x / pi)
    int si = (int)(x * 0.31830988f);
    x = x - (float)si * std::numbers::pi_v<float>;
    if (si & 1) {
        x = x > 0.0f ? x - std::numbers::pi_v<float> : x + std::numbers::pi_v<float>;
    }
    return 1.0f + x * x * internal::f2(x * x);
}

// 只有-2pi < x < 2pi时才会比std::sin快一点点（指不到1.00倍）
static inline constexpr std::complex<float> FastPolar(float x) {
    // si = (int)(x / pi)
    int si = (int)(x * 0.31830988f);
    x = x - (float)si * std::numbers::pi_v<float>;
    if (si & 1) {
        x = x > 0.0f ? x - std::numbers::pi_v<float> : x + std::numbers::pi_v<float>;
    }
    float const sin_x = x + x * x * x * internal::f1(x * x);
    float const cos_x = 1.0f + x * x * internal::f2(x * x);
    return {cos_x, sin_x};
}
}
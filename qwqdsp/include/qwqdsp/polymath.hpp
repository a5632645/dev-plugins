#pragma once
#include <cmath>
#include <numbers>
#include <complex>

namespace qwqdsp::polymath {
/**
 * @param x [0, pi]
 */
static inline constexpr float SinPi(float x) noexcept {
    float const x2 = x * x;
    float u = 1.3528548e-10f;
    u = u * x2 + -2.4703144e-08f;
    u = u * x2 + 2.7532926e-06f;
    u = u * x2 + -0.00019840381f;
    u = u * x2 + 0.0083333179f;
    float f = u * x2 + -0.16666666f;
    return x * x2 * f + x;
}

/**
 * @param x [-pi, pi]
 */
static inline constexpr float SinCycle(float x) noexcept {
    return std::copysign(SinPi(std::abs(x)), x);
}

/**
 * @param x [0, 2pi]
 */
static inline constexpr float Sin2Pi(float x) noexcept {
    return -SinCycle(x - std::numbers::pi_v<float>);
}

/**
 * @param x [0, pi]
 */
static inline constexpr float CosPi(float x) noexcept {
    float const x2 = x * x;
    float u = 1.7290616e-09f;
    u = u * x2 + -2.7093486e-07f;
    u = u * x2 + 2.4771643e-05f;
    u = u * x2 + -0.0013887906f;
    u = u * x2 + 0.041666519f;
    float const f = u * x2 + -0.49999991f;
    return 1 + x2 * f;
}

/**
 * @param x [-pi, pi]
 */
static inline constexpr float CosCycle(float x) noexcept {
    return CosPi(std::abs(x));
}

/**
 * @param x [0, 2pi]
 */
static inline constexpr float Cos2Pi(float x) noexcept {
    return -CosCycle(x - std::numbers::pi_v<float>);
}

/**
 * @param x [0, pi]
 */
static inline constexpr std::complex<float> PolarPi(float x) noexcept {
    float const x2 = x * x;
    float u = 1.3528548e-10f;
    u = u * x2 + -2.4703144e-08f;
    u = u * x2 + 2.7532926e-06f;
    u = u * x2 + -0.00019840381f;
    u = u * x2 + 0.0083333179f;
    float f = u * x2 + -0.16666666f;
    float const sin = x * x2 * f + x;

    u = 1.7290616e-09f;
    u = u * x2 + -2.7093486e-07f;
    u = u * x2 + 2.4771643e-05f;
    u = u * x2 + -0.0013887906f;
    u = u * x2 + 0.041666519f;
    f = u * x2 + -0.49999991f;
    float const cos = 1 + x2 * f;

    return {cos, sin};
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
static inline constexpr T SinRemez(T x) noexcept {
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
static constexpr inline T SinRemezRat(T x) noexcept {
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

/**
 * @brief poly sin approximate from reaktor, -110dB 3rd harmonic
 * @note x from 0.5 is sin, 0 is cos
 * @param x [0.0, 1.0]
 */
static inline constexpr float SinReaktor(float x) noexcept {
    x = 2 * std::abs(x - 0.5f) - 0.5f;
    float const x2 = x * x;
    float u = -0.540347434104161f * x2 + 2.535656174488765f;
    u = u * x2 -5.166512943349853f;
    u = u * x2 + 3.141592653589793f;
    return u * x;
}

/**
 * 12.071x cycle
 * @ref https://math.stackexchange.com/questions/107292/rapid-approximation-of-tanhx
 * @param x [-5,+5] have a error less than -80dB
 */
static inline constexpr float TanhSlow(float x) noexcept {
    float x2 = x * x;
    float a = x * (135135.0f + x2 * (17325.0f + x2 * (378.0f + x2)));
    float b = 135135.0f + x2 * (62370.0f + x2 * (3150.0f + x2 * 28.0f));
    return a / b;
}

/**
 * 7.757x cycle
 * @ref https://math.stackexchange.com/questions/107292/rapid-approximation-of-tanhx
 * @param x [-1,+1] have a error less than -70dB
 */
static inline constexpr float TanhFastest(float x) noexcept {
    float x2 = x * x;
    return x / (1 + x2 / (3 + x2 * 0.2f));
}

/**
 * 10.918x cycle
 * @ref https://math.stackexchange.com/questions/107292/rapid-approximation-of-tanhx
 * @param x [-5.5,+5.5] have a error less than -70dB
 */
static inline constexpr float TanhFast(float x) noexcept {
    float x2 = x * x;
    float x4 = x2 * x2;
    float x6 = x4 * x2;
    float up = x * (10 + x2) * (60 + x2);
    float down = 600 + 270 * x2 + 11 * x4 + x6 / 24;
    return up / down;
}

/**
 * 8.122x cycle -Ofast, slower than Pade -O2
 * @ref https://math.stackexchange.com/questions/490652/about-a-function-approximating-the-arctanx
 * @param x a peak -40dB erro at 3.1
 */
static inline constexpr float ArctanFast(float x) noexcept {
    constexpr float kGainNorm = std::numbers::pi_v<float>/2;
    constexpr float m = 16/std::numbers::pi_v<float>;
    constexpr float m2 = m * m;
    return 8*kGainNorm*x/(3+std::sqrt(25+m2*x*x));
}

/**
 * 9.064x cycle
 * @ref https://pmc.ncbi.nlm.nih.gov/articles/PMC5285437/
 * @param x [-1,1] have a error less than -59dB
 * @note 这个函数不是发散而是有界的,inf趋近于0
 *       最大值 (2.64575, 1.1024)
 */
static inline constexpr float ArctanPade(float x) noexcept {
    float x2 = x * x;
    float x3 = x2 * x;
    float x4 = x2 * x2;
    return (55*x3+105*x)/(9*x4+90*x2+105);
}
}
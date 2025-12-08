#pragma once
#include <cmath>
#include <numbers>
#include <complex>

namespace qwqdsp_oscillator::blep_coeff {
static constexpr float x2(float x) noexcept {
    return x * x;
}
static constexpr float x3(float x) noexcept {
    return x2(x) * x;
}
static constexpr float x4(float x) noexcept {
    return x2(x) * x2(x);
}
static constexpr float x5(float x) noexcept {
    return x2(x) * x3(x);
}

struct Triangle {
    static constexpr float kHalfLen = 1.0f;
    /**
     * @param x[0..kHalfLen]
     */
    static constexpr float GetBlepHalf(float x) noexcept {
        float v = x - 1.0f;
        return -v * v * 0.5f;
    }

    /**
     * @param x[0..kHalfLen]
     */
    static constexpr float GetBlampHalf(float x) noexcept {
        float v = x - 1;
        return -v * v * v / 6;
    }
};

struct Hann {
    static constexpr float kHalfLen = 2.0f;
    static float GetBlepHalf(float x) noexcept {
        constexpr float half_pi = std::numbers::pi_v<float> / 2;
        constexpr float inv_twopi = 1.0f / (std::numbers::pi_v<float> * 2);
        return x * 0.25f + std::sin(half_pi * x) * inv_twopi - 0.5f;
    }

    static float GetBlampHalf(float x) noexcept {
        constexpr float inv_pi2 = 1.0f / x2(std::numbers::pi_v<float>);
        constexpr float half_pi = std::numbers::pi_v<float> / 2;
        return (1.0f - x) / 2.0f + x2(x) / 8.0f - inv_pi2 * (1.0f + std::cos(half_pi * x));
    }
};

/**
 * @ref https://ccrma.stanford.edu/~juhan/vas.html
 * @ref https://www.researchgate.net/publication/307990687_Rounding_Corners_with_BLAMP
 */
struct BSpline {
    static constexpr float kHalfLen = 2.0f;
    static constexpr float GetBlepHalf(float x) noexcept {
        const float is_less_than_one_mask = x - 1.0f; 
        const float result_if_less = (x2(x) - 2.0f) * (6.0f - 8.0f * x + 3.0f * x2(x)) / 24.0f;
        const float result_if_greater = -x4(x - 2.0f) / 24.0f;
        return (is_less_than_one_mask < 0.0f) 
            ? result_if_less 
            : result_if_greater;
    }

    static constexpr float GetBlampHalf(float x) noexcept {
        const float is_less_than_one_mask = x - 1.0f; 
        const float result_if_less = (28.0f - 60.0f * x + 40.0f * x2(x) - 10.0f * x4(x) + 3.0f * x5(x)) / 120.0f;
        const float result_if_greater = -x5(x - 2.0f) / 120.0f;
        return (is_less_than_one_mask < 0.0f) 
            ? result_if_less 
            : result_if_greater;
    }
};

struct BlackmanNutall {
    static constexpr float kHalfLen = 3.0f;
    static float GetBlepHalf(float x) noexcept {
        constexpr auto invpi = std::numbers::inv_pi_v<float>;
        constexpr auto pi = std::numbers::pi_v<float>;
        constexpr auto a1 = 0.166666666666667f;
        constexpr auto a2 = 0.672719819110907f * invpi;
        constexpr auto a3 = 0.0939262240502071f * invpi;
        constexpr auto a4 = 0.00487790142101867f * invpi;

        auto c = std::polar(1.0f, x * (pi / 3));
        auto r = c;

        float y = 0;
        y += a1 * x;
        y += a2 * r.imag();
        r *= c;
        y += a3 * r.imag();
        r *= c;
        y += a4 * r.imag();
        r *= c;
        y -= 0.5f;
        return y;
    }

    static float GetBlampHalf(float x) noexcept {
        const float is_less_than_one_mask = x - 1.0f; 
        const float result_if_less = (28.0f - 60.0f * x + 40.0f * x2(x) - 10.0f * x4(x) + 3.0f * x5(x)) / 120.0f;
        const float result_if_greater = -x5(x - 2.0f) / 120.0f;
        return (is_less_than_one_mask < 0.0f) 
            ? result_if_less 
            : result_if_greater;
    }
};

struct BlackmanNutallApprox {
    static constexpr float kHalfLen = 3.0f;
    static float GetBlepHalf(float x) noexcept {
        auto x2_ = x2(x);
        auto x3_ = x3(x);
        auto x4_ = x4(x);
        // auto x5_ = x5(x);
        // auto up = -2.7673817498867354050e-16f + 0.45840339972415986730f * x
        //     - 0.16891181699025579563f * x2_ + 0.037114527838740719608f * x3_
        //     + 0.0051536787792799141191f * x4_ + 0.000054135383295800410139f * x5_;
        // auto down = 1 - 0.36835659332475467968f * x + 0.28691446911539841256f * x2_
        //     - 0.062945304501047083382f * x3_ + 0.022194671162805943679f * x4_;
        // return up / down - 0.5f;
        auto up = -6.1021365199045747765e-15f + 0.45843297259388023674f * x
            - 0.21663097430062709809f * x2_ + 0.045461384847873654394f * x3_;
        auto down = 1 - 0.47068729933220605095f * x + 0.29689585628509952473f * x2_
            - 0.081682128563515880655f * x3_ + 0.015452559765959259491f * x4_;
        return up / down - 0.5f;
    }

    static float GetBlampHalf(float x) noexcept {
        auto x2_ = x2(x);
        auto x3_ = x3(x);
        auto x4_ = x4(x);
        auto up = 0.34004f - 0.460155f * x + 0.23447f * x2_ - 0.0533133f * x3_ + 0.0045636f * x4_;
        auto down = 1 + 0.116039f * x + 0.196071f * x2_ + 0.0203012f * x3_ + 0.0337397f * x4_;
        return up / down;
    }
};

template<class T>
concept CBlepCoeff = requires (float x) {
    {T::GetBlepHalf(x)} -> std::same_as<float>;
    {T::GetBlampHalf(x)} -> std::same_as<float>;
    T::kHalfLen;
};
}

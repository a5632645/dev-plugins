#pragma once
#include <array>
#include <complex>
#include <cstddef>
#include "traditional_design.hpp"

namespace qwqdsp {
template<size_t kNumFilter>
class ComplexHalfband {
public:
    ComplexHalfband() {
        auto protyle = TraditionalDesign::Chebyshev2(kNumFilter, -100.0f);
        protyle = TraditionalDesign::Bilinear(protyle, 0.5);
        double k = std::pow(protyle.k, 1.0 / kNumFilter);
        for (int i = 0; i < protyle.size(); ++i) {
            auto& f = filters_[i];
            f.b0_ = k;
            f.b2_ = -std::norm(*protyle[i].z) * k;
            f.b1_.imag(-2.0f * std::real(*protyle[i].z) * k);
            f.b1_.real(0.0f);
            f.a2_ = -std::norm(protyle[i].p);
            f.a1_.imag(-2.0f * std::real(protyle[i].p));
            f.a1_.real(0.0f);
        }
    }

    std::complex<float> Tick(std::complex<float> x) noexcept {
        for (auto& f : filters_) {
            x = f.Tick(x);
        }
        return x;
    }
private:
    struct ComplexBiquad {
        float b0_{};
        std::complex<float> b1_{};
        float b2_{};
        std::complex<float> a1_{};
        float a2_{};
        std::complex<float> latch1_{};
        std::complex<float> latch2_{};

        std::complex<float> Tick(std::complex<float> x) noexcept {
            auto output = x * b0_ + latch1_;
            latch1_ = x * b1_ - output * a1_ + latch2_;
            latch2_ = x * b2_ - output * a2_;
            return output;
        }
    };
    std::array<ComplexBiquad, kNumFilter> filters_;
};
}
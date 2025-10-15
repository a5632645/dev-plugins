#pragma once
#include <numbers>
#include <cmath>

namespace qwqdsp::filter {
template<class SimdType>
class OnePoleTPTSimd {
public:
    void Reset() noexcept {
        lag_ = SimdType::FromSingle(0);
    }
    
    static float ComputeCoeff(float w) noexcept {
        constexpr float kMaxOmega = std::numbers::pi_v<float> - 1e-5f;
        [[unlikely]]
        if (w < 0.0f) {
            return 0.0f;
        }
        else if (w > kMaxOmega) {
            return 1.0f;
        }
        else [[likely]] {
            auto k = std::tan(w / 2);
            return k / (1 + k);
        }
    }

    static SimdType ComputeCoeffs(SimdType const& w) noexcept {
        SimdType r;
        for (size_t i = 0; i < SimdType::kSize; ++i) {
            r.x[i] = ComputeCoeff(w.x[i]);
        }
        return r;
    }

    SimdType TickLowpass(SimdType const& x, SimdType const& coeff) noexcept {
        SimdType delta = coeff * (x - lag_);
        lag_ += delta;
        SimdType y = lag_;
        lag_ += delta;
        return y;
    }

    SimdType TickHighpass(SimdType const& x, SimdType const& coeff) noexcept {
        SimdType delta = coeff * (x - lag_);
        lag_ += delta;
        SimdType y = lag_;
        lag_ += delta;
        return x - y;
    }

    SimdType TickHighshelf(SimdType const& x, SimdType const& coeff, SimdType const& gain) noexcept {
        SimdType lp = TickLowpass(x, coeff);
        return lp + gain * (x - lp);
    }

    SimdType TickAllpass(SimdType const& x, SimdType const& coeff) noexcept {
        SimdType lp = TickLowpass(x, coeff);
        lp += lp;
        return lp - x;
    }
private:
    SimdType lag_{};
};
}
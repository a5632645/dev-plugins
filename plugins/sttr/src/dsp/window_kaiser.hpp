#pragma once
#include <cmath>

#include "pluginshared/simd/inst.hpp"
#include "pluginshared/simd/simd.hpp"

namespace sttr {

// ------------------------------------------------------------
/** Kaiser window function — evaluates at a normalised position t in [0, 1].

    The Kaiser window is controlled by a single beta parameter; the window
    length is controlled separately by DspImpl::windowMul.

    Templated on (inst, SimdT) so each ISA variant is a distinct type — this
    avoids ODR-sharing code that is compiled with different instruction sets.
 */
template <simd::Inst inst, class SimdT>
class Window {
public:
    Window() = default;

    void SetBeta(float beta) noexcept {
        beta_ = beta;
        inv_i0_beta_ = 1.0f / BesselI0(beta_);
    }

    float Beta() const noexcept {
        return beta_;
    }

    /** Kaiser window value at normalised position t in [0, 1]. */
    float Value(float t) const noexcept {
        // w(t) = I0(beta * sqrt(1 - (2t - 1)^2)) / I0(beta)
        float const arg = 1.0f - (2.0f * t - 1.0f) * (2.0f * t - 1.0f);
        if (arg <= 0.0f) return 1.0f;
        return BesselI0(beta_ * std::sqrt(arg)) * inv_i0_beta_;
    }

    /** Kaiser window values at 4 normalised positions t in [0, 1]. */
    simd::Float128 Value(simd::Float128 t) const noexcept {
        simd::Float128 const one = simd::BroadcastF128(1.0f);
        simd::Float128 const two = simd::BroadcastF128(2.0f);
        simd::Float128 const zero = simd::Float128{};

        // arg = 1 - (2t - 1)^2; clamp to >= 0 so sqrt stays finite
        simd::Float128 const arg = one - (two * t - one) * (two * t - one);
        simd::Float128 const computed =
            BesselI0(beta_ * simd::Sqrt(simd::Max(arg, zero))) * simd::BroadcastF128(inv_i0_beta_);

        // arg <= 0 (outside [0, 1]) -> 1.0, otherwise computed
        simd::Float128 const maskf = simd::ToFloat(arg > zero); // 0.0 or -1.0 per lane
        return computed * (-maskf) + one * (maskf + one);
    }
private:
    // Padé (8,8) coefficients of exp(-x)*I0(x), ascending powers of x
    static constexpr float kNum[] = {
        1.0f,                   // x^0
        0.105679f,              // x^1
        0.232432f,              // x^2
        0.022871f,              // x^3
        0.0113535f,             // x^4
        0.000831911f,           // x^5
        0.000176505f,           // x^6
        7.559388122773327e-6f,  // x^7
        2.3196902640592364e-8f, // x^8
    };
    static constexpr float kDen[] = {
        1.0f,                  // x^0
        1.1057f,               // x^1
        0.588013f,             // x^2
        0.198596f,             // x^3
        0.0468095f,            // x^4
        0.00842545f,           // x^5
        0.000960233f,          // x^6
        0.000120437f,          // x^7
        1.612947555525283e-6f, // x^8
    };

    /** Zeroth-order modified Bessel function of the first kind (scalar).
        Padé (8,8) rational approximation of exp(-x) * I0(x), valid for
        x in [0, 16] (max beta = 16, and beta * sqrt(1 - (2t-1)^2) <= beta);
        the result is multiplied by exp(x) to recover I0(x). */
    static float BesselI0(float x) noexcept {
        // Horner evaluation of the Padé approximant of exp(-x) * I0(x)
        float num = kNum[8];
        float den = kDen[8];
        for (int i = 7; i >= 0; --i) {
            num = num * x + kNum[i];
            den = den * x + kDen[i];
        }
        return std::exp(x) * (num / den);
    }

    /** Zeroth-order modified Bessel function of the first kind (4-lane SIMD). */
    static simd::Float128 BesselI0(simd::Float128 x) noexcept {
        simd::Float128 num = simd::BroadcastF128(kNum[8]);
        simd::Float128 den = simd::BroadcastF128(kDen[8]);
        for (int i = 7; i >= 0; --i) {
            num = num * x + simd::BroadcastF128(kNum[i]);
            den = den * x + simd::BroadcastF128(kDen[i]);
        }
        return ExpF(x) * (num / den);
    }

    static simd::Float128 ExpF(simd::Float128 x) noexcept {
        return simd::Float128{std::exp(x[0]), std::exp(x[1]), std::exp(x[2]), std::exp(x[3])};
    }

    float beta_{8.0f};
    float inv_i0_beta_{1.0f / BesselI0(8.0f)};
};

} // namespace sttr

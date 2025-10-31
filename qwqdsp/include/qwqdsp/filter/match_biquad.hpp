#pragma once
#include <algorithm>
#include <cmath>
#include <numbers>
#include <array>
#include <numeric>
#include "qwqdsp/filter/biquad_coeff.hpp"
#include "qwqdsp/filter/analog_responce.hpp"
/**
 * @ref 全通 https://apulsoft.ch/blog/matched-allpass/
 * @ref 复极点shelf https://vicanek.de/articles/2poleShelvingFits.pdf
 * @ref 单极点shelf https://vicanek.de/articles/ShelvingFits.pdf
 * @ref 经典 https://vicanek.de/articles/BiquadFits.pdf
 */
namespace qwqdsp::filter {
class MatchBiquad {
public:
    // -------------------- onepole --------------------
    BiquadCoeff HighshelfOnepole(float wc, float db) noexcept {
        auto G = std::pow(10.0, db / 20.0);
        auto fc = wc / kPi;
        auto alpha = 2.0 / (kPi * kPi) * (1.0 + 1.0 / (G * fc * fc)) - 0.5;
        auto beta = 2.0 / (kPi * kPi) * (1.0 + G / (fc * fc)) - 0.5;
        auto a1 = -alpha / (1.0 + alpha + ClampSqrt(1.0 + 2.0 * alpha));
        auto b = -beta / (1.0 + beta + ClampSqrt(1.0 + 2.0 * beta));
        auto b0 = (1.0 + a1) / (1.0 + b);
        auto b1 = b * b0;
        return DoubleBiquadCoeff{b0,b1,0,a1,0}.ToFloat();
    }

    BiquadCoeff TiltshelfOnepole(float wc, float db) noexcept {
        auto sqrt_G = std::pow(10.0f, -db / 40.0f);
        auto r = HighshelfOnepole(wc, db);
        r.b0 *= sqrt_G;
        r.b1 *= sqrt_G;
        r.b2 *= sqrt_G;
        return r;
    }

    BiquadCoeff LowshelfOnepole(float wc, float db) noexcept {
        auto G = std::pow(10.0f, db / 20.0f);
        auto r = HighshelfOnepole(wc, -db);
        r.b0 *= G;
        r.b1 *= G;
        r.b2 *= G;
        return r;
    }

    /**
     * @note match pi/2和双线性变换一样
     */
    template<double kMatchPhase = std::numbers::pi/4>
    BiquadCoeff AllpassOnepole(float wc) noexcept {
        static auto const cos_a = std::cos(kMatchPhase);
        static auto const f_a = std::sqrt((1 - cos_a) / (1 + cos_a));
        auto w_a = f_a * wc;
        auto c_0 = (std::cos(w_a) - cos_a) / (std::cos(kMatchPhase + w_a) - 1);
        return DoubleBiquadCoeff{c_0,1,0,c_0,0}.ToFloat();
    }

    // -------------------- twopole --------------------
    BiquadCoeff Lowpass(float wc, float Q) noexcept {
        auto r = ImpluseInvarant(wc, Q);
        auto phi = GetPhi(wc);
        auto A = GetA(r);
        auto R = Q * Q * std::inner_product(phi.begin(), phi.end(), A.begin(), double{});
        auto B0 = A[0];
        auto B1 = (R - B0 * phi[0]) / phi[1];
        B1 = std::max(B1, 0.0);
        auto sqrt_B0 = 1 + r.a1 + r.a2;
        auto sqrt_B1 = std::sqrt(B1);
        r.b0 = (sqrt_B0 + sqrt_B1) / 2;
        r.b1 = sqrt_B0 - r.b0;
        r.b2 = 0;
        return r.ToFloat();
    }

    BiquadCoeff Highpass(float wc, float Q) noexcept {
        auto r = ImpluseInvarant(wc, Q);
        auto phi = GetPhi(wc);
        auto A = GetA(r);
        auto R = std::inner_product(phi.begin(), phi.end(), A.begin(), double{});
        R = std::max(R, 0.0);
        r.b0 = std::sqrt(R) * Q / (4 * phi[1]);
        r.b1 = -2 * r.b0;
        r.b2 = r.b0;
        return r.ToFloat();
    }

    BiquadCoeff NormBandpass(float wc, float Q) noexcept {
        auto r = ImpluseInvarant(wc, Q);
        auto phi = GetPhi(wc);
        auto A = GetA(r);
        auto R1 = std::inner_product(phi.begin(), phi.end(), A.begin(), double{});
        auto R2 = -A[0] + A[1] + 4 * (phi[0] - phi[1]) * A[2];
        auto B2 = (R1 - R2 * phi[1]) / (4 * phi[1] * phi[1]);
        auto B1 = R2 + 4 * (phi[1] - phi[0]) * B2;
        r.b1 = -std::sqrt(std::max(B1,0.0)) / 2;
        r.b0 = (std::sqrt(std::max(0.0,B2 + B1 / 4)) - r.b1) / 2;
        r.b2 = -r.b0 - r.b1;
        return r.ToFloat();
    }

    BiquadCoeff Bandpass(float wc, float Q) noexcept {
        auto r = ImpluseInvarant(wc, Q);
        auto phi = GetPhi(wc);
        auto A = GetA(r);
        auto R1 = Q * Q * std::inner_product(phi.begin(), phi.end(), A.begin(), double{});
        auto R2 = Q * Q * (-A[0] + A[1] + 4 * (phi[0] - phi[1]) * A[2]);
        auto B2 = (R1 - R2 * phi[1]) / (4 * phi[1] * phi[1]);
        auto B1 = R2 + 4 * (phi[1] - phi[0]) * B2;
        r.b1 = -std::sqrt(std::max(B1,0.0)) / 2;
        r.b0 = (std::sqrt(std::max(0.0,B2 + B1 / 4)) - r.b1) / 2;
        r.b2 = -r.b0 - r.b1;
        return r.ToFloat();
    }

    BiquadCoeff Notch(float wc, float Q) noexcept {
        auto r = ImpluseInvarant(wc, Q);
        auto phi = GetPhi(wc);
        auto A = GetA(r);
        auto B0 = A[0];
        auto B2 = (-B0) / (4 * phi[1]*phi[1]);
        auto B1 = B0 + 4 * (phi[1]-phi[0])*B2;
        Solveb(r, B0, B1, B2);
        return r.ToFloat();
    }

    BiquadCoeff Peaking(float wc, float Q, float db) noexcept {
        auto G = std::pow(10.0f, db / 20.0f);
        auto r = ImpluseInvarant(wc, Q);
        auto phi = GetPhi(wc);
        auto A = GetA(r);
        auto R1 = G * G * std::inner_product(phi.begin(), phi.end(), A.begin(), double{});
        auto R2 = G * G * (-A[0] + A[1] + 4 * (phi[0] - phi[1]) * A[2]);
        auto B0 = A[0];
        auto B2 = (R1 - R2 * phi[1] - B0) / (4 * phi[1]*phi[1]);
        auto B1 = R2 + B0 + 4 * (phi[1]-phi[0])*B2;
        Solveb(r, B0, B1, B2);
        return r.ToFloat();
    }

    BiquadCoeff Highshelf(float wc, float Q, float db) noexcept {
        AnalogResponce analog_;
        auto sqrt_G = std::pow(10.0f, db / 80.0f);
        auto r = ImpluseInvarant(wc * sqrt_G, Q);
        auto A = GetA(r);
        auto B0 = A[0];
        auto h_pow2_pi = std::norm(analog_.Highshelf(kPi, wc, Q, sqrt_G));
        auto B1 = A[1] * h_pow2_pi;
        auto phi = GetPhi(wc);
        auto h_pow2_wc = std::norm(analog_.Highshelf(wc, wc, Q, sqrt_G));
        auto R1 = h_pow2_wc * std::inner_product(phi.begin(), phi.end(), A.begin(), double{});
        auto B2 = (R1-B0*phi[0]-B1*phi[1])/phi[2];
        Solveb(r, B0, B1, B2);
        return r.ToFloat();
    }

    BiquadCoeff Lowshelf(float wc, float Q, float db) noexcept {
        AnalogResponce analog_;
        auto sqrt_G = std::pow(10.0f, db / 80.0f);
        auto r = ImpluseInvarant(wc/sqrt_G, Q);
        auto A = GetA(r);
        auto B0 = A[0] * std::norm(analog_.Lowshelf(0, wc, Q, sqrt_G));
        auto h_pow2_pi = std::norm(analog_.Lowshelf(kPi, wc, Q, sqrt_G));
        auto B1 = A[1] * h_pow2_pi;
        auto phi = GetPhi(wc);
        auto h_pow2_wc = std::norm(analog_.Lowshelf(wc, wc, Q, sqrt_G));
        auto R1 = h_pow2_wc * std::inner_product(phi.begin(), phi.end(), A.begin(), double{});
        auto B2 = (R1-B0*phi[0]-B1*phi[1])/phi[2];
        Solveb(r, B0, B1, B2);
        return r.ToFloat();
    }

    BiquadCoeff Tiltshelf(float wc, float Q, float db) noexcept {
        AnalogResponce analog_;
        auto sqrt_G = std::pow(10.0f, db / 80.0f);
        auto r = ImpluseInvarant(wc * sqrt_G, Q);
        auto A = GetA(r);
        auto B0 = A[0] * std::norm(analog_.Tiltshelf(0, wc, Q, sqrt_G));
        auto h_pow2_pi = std::norm(analog_.Tiltshelf(kPi, wc, Q, sqrt_G));
        auto B1 = A[1] * h_pow2_pi;
        auto phi = GetPhi(wc);
        auto h_pow2_wc = std::norm(analog_.Tiltshelf(wc, wc, Q, sqrt_G));
        auto R1 = h_pow2_wc * std::inner_product(phi.begin(), phi.end(), A.begin(), double{});
        auto B2 = (R1-B0*phi[0]-B1*phi[1])/phi[2];
        Solveb(r, B0, B1, B2);
        return r.ToFloat();
    }

    BiquadCoeff Allpass(float wc, float Q) noexcept {
        auto f_c = wc / (kPi * 2);
        auto w_c = wc;
        // zeta damping factor
        auto zeta = 1 / (2 * Q); 
        auto zetasq = zeta * zeta;

        // match phases at f_c and the point where the phase is -a.
        auto a = std::clamp(std::sqrt(zeta / (2 * f_c)), 0.01, 1.5);
        auto cos_a = std::cos(a);
        auto A = cos_a + 1;
        auto sin_a_h = std::sqrt(0.5 - 0.5 * cos_a);
        auto cos_a_h = std::sqrt(0.5 + 0.5 * cos_a);

        auto R = (A * (zetasq - 1) + 2) * A;
        auto B = -2 * zetasq * A + 2 * zeta * std::sqrt(R) + A - 2;
        auto f_a = std::sqrt(B / (cos_a - 1));

        // create digital allpass through two points
        auto w_a = 2 * kPi * std::min(0.499, f_c * f_a);

        auto cos_w_c = std::cos(w_c);
        auto sin_w_a = std::sin(w_a), cos_w_a = std::cos(w_a);

        auto C = sin_a_h * (cos_w_a - cos_w_c);
        auto D = sin_w_a * cos_a_h;

        auto bot = -1 / (C + D);
        auto c_0 = (C - D) * bot;
        auto c_1 = 2 * cos_w_c * D * bot;
        return DoubleBiquadCoeff{c_0,c_1,1,c_1,c_0}.ToFloat();
    }
private:
    static constexpr auto kPi = std::numbers::pi_v<double>;

    static inline DoubleBiquadCoeff ImpluseInvarant(float wc, float Q) noexcept {
        auto zeta = 1 / (2 * Q);
        auto exp_qwc = std::exp(-zeta * wc);
        auto a1 = float{};
        if (zeta <= 1) {
            a1 = -2 * exp_qwc * std::cos(std::sqrt(1 - zeta*zeta) * wc);
        }
        else {
            a1 = -2 * exp_qwc * std::cosh(std::sqrt(zeta*zeta - 1) * wc);
        }
        auto a2 = exp_qwc * exp_qwc;
        return DoubleBiquadCoeff{0,0,0,a1,a2};
    }

    static inline void Solveb(DoubleBiquadCoeff& c, float B0, float B1, float B2) noexcept {
        auto sqrt_B0 = ClampSqrt(B0);
        auto sqrt_B1 = ClampSqrt(B1);
        auto W = (sqrt_B0 + sqrt_B1) / 2;
        c.b0 = (W + ClampSqrt(W*W + B2)) / 2;
        c.b1 = (sqrt_B0 - sqrt_B1) / 2;
        c.b2 = -B2 / (4*c.b0);
    }

    static inline std::array<double, 3> GetA(DoubleBiquadCoeff const& c) noexcept {
        auto a1 = c.a1;
        auto a2 = c.a2;
        return {X2(1+a1+a2), X2(1-a1+a2), -4*a2};
    }

    static inline std::array<double, 3> GetPhi(double w) noexcept {
        auto sin2 = X2(std::sin(w / 2));
        auto phi0 = 1 - sin2;
        auto phi1 = sin2;
        return {phi0, phi1, 4*phi0*phi1};
    }

    static inline constexpr double X2(double x) noexcept {
        return x * x;
    }

    static inline double ClampSqrt(double x) noexcept {
        return std::sqrt(std::max(x, 0.0));
    }
};
}
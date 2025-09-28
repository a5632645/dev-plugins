#pragma once
#include "qwqdsp/filter/iir_design.hpp"

namespace qwqdsp::filter {
class IIRDesignExtra {
public:
    using ZPK = IIRDesign::ZPK;
    static constexpr auto pi = IIRDesign::pi;

    static constexpr double AttenGain2SquareEpsi(double atten) noexcept {
        return (1.0f - atten * atten) / atten;
    }

    static double AttenDb2SquareEpsi(double atten) noexcept {
        return std::pow(10.0, atten / 10.0) - 1.0;
    }

    /**
     * @param atten (0, 1)
     */
    static double ButterworthAttenGain(std::span<ZPK> ret, size_t num_filter, double atten) {
        return ButterworthAtten(ret, num_filter, (1.0f - atten * atten) / atten);
    }

    /**
     * @param atten >0
     */
    static double ButterworthAttenDb(std::span<ZPK> ret, size_t num_filter, double atten) {
        return ButterworthAtten(ret, num_filter, std::pow(10.0, atten / 10.0) - 1.0);
    }

    /**
     * @brief (-atten)dB at (1)rad/sec
     * @param ripple (>0)dB
     * @param atten (>0)dB
     */
    static double Chebyshev1(std::span<ZPK> ret, size_t num_filter, double ripple, double atten, bool even_pole_modify) {
        assert(ret.size() >= num_filter);
        assert(atten >= ripple);

        size_t const n = 2 * num_filter;
        double first_pole = std::cos(pi * (n - 1.0) / (2.0 * n));
        first_pole = first_pole * first_pole;

        double scale = 0.0;
        if (!even_pole_modify) {
            if (atten >= ripple) {
                scale = 1.0 / std::cosh(std::acosh(std::sqrt((std::pow(10.0, atten / 10.0) - 1.0) / (std::pow(10.0, ripple / 10.0) - 1.0))) / static_cast<double>(n));
            }
            else {
                scale = 1.0 / std::cos(std::acos(std::sqrt((std::pow(10.0, atten / 10.0) - 1.0) / (std::pow(10.0, ripple / 10.0) - 1.0))) / static_cast<double>(n));
            }
        }
        else {
            if (atten >= ripple) {
                scale = std::cosh(std::acosh(std::sqrt((std::pow(10.0, atten / 10.0) - 1.0) / (std::pow(10.0, ripple / 10.0) - 1.0))) / static_cast<double>(n));
            }
            else {
                scale = std::cos(std::acos(std::sqrt((std::pow(10.0, atten / 10.0) - 1.0) / (std::pow(10.0, ripple / 10.0) - 1.0))) / static_cast<double>(n));
            }
            scale *= scale;
            scale = std::sqrt((1.0 - first_pole) / (scale - first_pole));
        }

        double const eps = std::sqrt(std::pow(10.0, ripple / 10.0) - 1.0);
        double const A = 1.0 / n * std::asinh(1.0 / eps);
        double const k_re = std::sinh(A);
        double const k_im = std::cosh(A);
        
        double gain = 1.0;
        size_t i = 0;
        for (size_t k = 1; k <= num_filter; ++k) {
            double phi = (2.0 * k - 1.0) * pi / (2.0 * n);
            if (even_pole_modify) {
                auto pole = std::complex{-std::sin(phi) * k_re, std::cos(phi) * k_im};
                ret[i].p = scale * std::sqrt((pole * pole + first_pole) / (1.0 - first_pole));
            }
            else {
                ret[i].p = scale * std::complex{-std::sin(phi) * k_re, std::cos(phi) * k_im};
            }
            gain *= std::norm(ret[i].p);
            ++i;
        }
        gain /= std::sqrt(1.0f + eps * eps);
        return gain;
    }

    /**
     * @brief (-atten)dB at (1)rad/sec
     * @param ripple (>0)dB
     * @param atten (>0)dB
     */
    static double Chebyshev2(std::span<ZPK> ret, size_t num_filter, double ripple, double atten, bool even_order_modify) {
        assert(ret.size() >= num_filter);

        size_t n = 2 * num_filter;
        double first_pole = std::cos(pi * (n - 1.0) / (2.0 * n));
        first_pole = first_pole * first_pole;

        double scale = 0.0;
        if (!even_order_modify) {
            if (atten < ripple) {
                scale = std::cosh(std::acosh(std::sqrt((std::pow(10.0, ripple / 10.0) - 1.0) / (std::pow(10.0, atten / 10.0) - 1.0))) / static_cast<double>(n));
            }
            else {
                scale = std::cos(std::acos(std::sqrt((std::pow(10.0, ripple / 10.0) - 1.0) / (std::pow(10.0, atten / 10.0) - 1.0))) / static_cast<double>(n));
            }
        }
        else {
            if (atten < ripple) {
                scale = std::cosh(std::acosh(std::sqrt((std::pow(10.0, ripple / 10.0) - 1.0) / (std::pow(10.0, atten / 10.0) - 1.0))) / static_cast<double>(n));
            }
            else {
                scale = std::cos(std::acos(std::sqrt((std::pow(10.0, ripple / 10.0) - 1.0) / (std::pow(10.0, atten / 10.0) - 1.0))) / static_cast<double>(n));
            }
            scale *= scale;
            scale = std::sqrt((scale - first_pole) / (1.0 - first_pole));
        }

        size_t i = 0;
        double eps = 1.0 / std::sqrt(std::pow(10.0, -ripple / 10.0) - 1.0);
        double A = 1.0 / n * std::asinh(1.0 / eps);
        double k_re = std::sinh(A);
        double k_im = std::cosh(A);
        double k = 1.0;

        // 最接近0的零点
        double const first_zero = std::cos((n / 2.0 - 1.0 + 0.5) * std::numbers::pi_v<double> / n);
        for (size_t k = 1; k <= num_filter; ++k) {
            double phi = (2.0 * k - 1.0) * pi / (2.0 * n);
            if (!even_order_modify) {
                ret[i].z = scale / std::complex{0.0, std::cos(phi)};
                ret[i].p = scale / std::complex{-std::sin(phi) * k_re, std::cos(phi) * k_im};
            }
            else {
                auto pole = std::complex{-std::sin(phi) * k_re, std::cos(phi) * k_im};
                ret[i].p = scale / std::sqrt((pole * pole + first_pole) / (1.0 - first_pole));
                if (k != num_filter) {
                    // 最靠近0的切比雪夫多项式的零点被映射到0，所以零点在无穷远处不赋值
                    double const zero = std::cos(phi);
                    double const tt = std::sqrt(std::max(0.0, (zero * zero - first_zero * first_zero) / (1.0 - first_zero * first_zero)));
                    ret[i].z = scale / std::complex{0.0, tt};
                }
            }
            if (ret[i].z) {
                k *= std::norm(ret[i].p) / std::norm(*ret[i].z);
            }
            else {
                k *= std::norm(ret[i].p);
            }
            ++i;
        }
        return k;
    }
private:
    static double ButterworthAtten(std::span<ZPK> ret, size_t num_filter, double square_epsi) {
        assert(ret.size() >= num_filter);

        double const g = 1.0 / std::pow(square_epsi, 0.25 / num_filter);
        size_t const n = 2 * num_filter;
        size_t i = 0;
        for (size_t k = 1; k <= num_filter; ++k) {
            double phi = (2.0 * k - 1.0) * pi / (2.0 * n);
            ret[i].p = g * std::complex{-std::sin(phi), std::cos(phi)};
            ++i;
        }
        return 1.0 / std::sqrt(square_epsi);
    }
};
}
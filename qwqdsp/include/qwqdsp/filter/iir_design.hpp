#pragma once
#include <optional>
#include <complex>
#include <span>
#include <numbers>
#include <vector>
#include <cassert>
#include "biquad_coeff.hpp"

namespace qwqdsp_filter {
struct IIRDesign {
    static constexpr auto pi = std::numbers::pi;

    struct ZPK {
        std::optional<std::complex<double>> z; // 如果Null则在无穷远处
        std::complex<double> p;
        double k;
    };

    static std::complex<double> ScaleComplex(const std::complex<double>& a, double b) {
        return {a.real() * b, a.imag() * b};
    }

    // --------------------------------------------------------------------------------
    // 原型滤波器
    // --------------------------------------------------------------------------------
    /**
     * @brief (-3.01)dB at (1)rad/sec
     */
    static void Butterworth(std::span<ZPK> ret, size_t num_filter) {
        assert(ret.size() >= num_filter);

        size_t n = 2 * num_filter;
        size_t i = 0;
        for (size_t k = 1; k <= num_filter; ++k) {
            double phi = (2.0 * static_cast<double>(k) - 1.0) * pi / (2.0 * static_cast<double>(n));
            ret[i].p = std::complex{-std::sin(phi), std::cos(phi)};
            ret[i].k = 1;
            ++i;
        }
    }

    /**
     * @brief (-ripple)dB at (1)rad/sec
     * @param ripple >0 dB
     * @ref https://en.wikipedia.org/wiki/Chebyshev_filter
     */
    static void Chebyshev1(std::span<ZPK> ret, size_t num_filter, double ripple, bool even_pole_modify) {
        assert(ret.size() >= num_filter);

        size_t n = 2 * num_filter;
        size_t i = 0;
        double eps = std::sqrt(std::pow(10.0, ripple / 10.0) - 1.0);
        double A = 1.0 / static_cast<double>(n) * std::asinh(1.0 / eps);
        double k_re = std::sinh(A);
        double k_im = std::cosh(A);
        double first_pole = std::cos(pi * (static_cast<double>(n) - 1.0) / (2.0 * static_cast<double>(n)));
        first_pole = first_pole * first_pole;
        for (size_t k = 1; k <= num_filter; ++k) {
            double phi = (2.0 * static_cast<double>(k) - 1.0) * pi / (2.0 * static_cast<double>(n));
            if (even_pole_modify) {
                auto pole = std::complex{-std::sin(phi) * k_re, std::cos(phi) * k_im};
                ret[i].p = std::sqrt((pole * pole + first_pole) / (1.0 - first_pole));
            }
            else {
                ret[i].p = std::complex{-std::sin(phi) * k_re, std::cos(phi) * k_im};
            }
            ret[i].k = std::norm(ret[i].p);
            ++i;
        }
        ret[0].k /= std::sqrt(1.0f + eps * eps);
    }

    /**
     * @brief (-3.01)dB at (1)rad/sec
     * @param ripple <0 dB
     * @ref https://en.wikipedia.org/wiki/Chebyshev_filter
     * @ref https://en.wikipedia.org/wiki/Chebyshev_nodes#Even_order_modified_Chebyshev_nodes
     */
    static void Chebyshev2(std::span<ZPK> ret, size_t num_filter, double ripple, bool even_order_modify) {
        assert(ret.size() >= num_filter);

        size_t n = 2 * num_filter;
        size_t i = 0;
        double eps = 1.0 / std::sqrt(std::pow(10.0, -ripple / 10.0) - 1.0);
        double A = 1.0 / static_cast<double>(n) * std::asinh(1.0 / eps);
        double scale = 1.0 / std::cosh(std::acosh(std::sqrt(std::pow(10.0, -ripple / 10.0) - 1.0)) / static_cast<double>(n));
        double k_re = std::sinh(A) * scale;
        double k_im = std::cosh(A) * scale;

        double first_pole = std::cos(pi * (static_cast<double>(n) - 1.0) / (2.0 * static_cast<double>(n)));
        first_pole = first_pole * first_pole;
        // 最接近0的零点
        double const first_zero = std::cos((static_cast<double>(n) / 2.0 - 1.0 + 0.5) * std::numbers::pi_v<double> / static_cast<double>(n));
        for (size_t k = 1; k <= num_filter; ++k) {
            double phi = (2.0 * static_cast<double>(k) - 1.0) * pi / (2.0 * static_cast<double>(n));
            if (!even_order_modify) {
                ret[i].z = 1.0 / std::complex{0.0, std::cos(phi) * scale};
                ret[i].p = 1.0 / std::complex{-std::sin(phi) * k_re, std::cos(phi) * k_im};
            }
            else {
                auto pole = std::complex{-std::sin(phi) * k_re, std::cos(phi) * k_im};
                ret[i].p = 1.0 / std::sqrt((pole * pole + first_pole) / (1.0 - first_pole));
                if (k != num_filter) {
                    // 最靠近0的切比雪夫多项式的零点被映射到0，所以零点在无穷远处不赋值
                    double const zero = std::cos(phi);
                    double const tt = std::sqrt(std::max(0.0, (zero * zero - first_zero * first_zero) / (1.0 - first_zero * first_zero)));
                    ret[i].z = 1.0 / std::complex{0.0, tt * scale};
                }
            }
            if (ret[i].z) {
                ret[i].k = std::norm(ret[i].p) / std::norm(*ret[i].z);
            }
            else {
                ret[i].k = std::norm(ret[i].p);
            }
            ++i;
        }
    }

    struct EllipticHelper {
        static constexpr double kError = std::numeric_limits<double>::epsilon();
        static constexpr bool kUseStd = false;

        EllipticHelper(double k0) {
            while (k0 > kError) {
                k_.push_back(k0);
                double kdot = std::sqrt(1.0 - k0 * k0);
                kdot_.push_back(kdot);
                k0 = (1.0 - kdot) / (1.0 + kdot);
            }
            k_.push_back(k0);
            kdot_.push_back(Kdot(k0));
        }

        double CompleteIntegral() {
            if constexpr (kUseStd) {
                return std::comp_ellint_1(k_.front());
            }
            else {
                const double a = std::sqrt(1.0 - 1.0e-3);
                const double k0 = k_.front();
                if (k0 <= a) {
                    double km = pi / 2.0;
                    for (size_t i = 1; i < k_.size(); ++i) {
                        double const k = k_[i];
                        km = km * (1 + k);
                    }
                    return km;
                }
                else {
                    const double kdot = kdot_.front();
                    double L = -std::log(kdot / 4.0);
                    return L + (L - 1.0) * (kdot * kdot) / 2.0;
                }
            }
        }

        std::complex<double> Cd(std::complex<double> u) {
            auto cdm = std::cos(u * pi / 2.0);
            for (size_t i = 0; i < k_.size() - 1; ++i) {
                double const k = k_[k_.size() - i - 1];
                cdm = (1.0 + k) / (1.0 / cdm + k * cdm);
            }
            return cdm;
        }

        double Cd(double u) {
            auto cdm = std::cos(u * pi / 2.0);
            for (size_t i = 0; i < k_.size() - 1; ++i) {
                double const k = k_[k_.size() - i - 1];
                cdm = (1.0 + k) / (1.0 / cdm + k * cdm);
            }
            return cdm;
        }

        std::complex<double> Sn(std::complex<double> u) {
            auto snm = std::sin(u * (pi / 2.0));
            for (size_t i = 0; i < k_.size() - 1; ++i) {
                double const k = k_[k_.size() - 1 - i];
                snm = (1.0 + k) / (1.0 / snm + k * snm);
            }
            return snm;
        }

        double Sn(double u) {
            auto snm = std::sin(u * (pi / 2.0));
            for (size_t i = 0; i < k_.size() - 1; ++i) {
                double const k = k_[k_.size() - 1 - i];
                snm = (1.0 + k) / (1.0 / snm + k * snm);
            }
            return snm;
        }

        std::complex<double> ArcSn(std::complex<double> sn0) {
            for (size_t i = 1; i < k_.size(); ++i) {
                sn0 = 2.0 * sn0 / ((1.0 + k_[i]) * (1.0 + std::sqrt(1.0 - k_[i - 1] * k_[i - 1] * sn0 * sn0)));
            }
            return std::asin(sn0) * (2.0 / pi);
        }

        double ArcSn(double sn0) {
            for (size_t i = 1; i < k_.size(); ++i) {
                sn0 = 2.0 * sn0 / ((1.0 + k_[i]) * (1.0 + std::sqrt(1.0 - k_[i - 1] * k_[i - 1] * sn0 * sn0)));
            }
            return std::asin(sn0) * (2.0 / pi);
        }

        static double Kdot(double k) {
            return std::sqrt(1.0 - k * k);
        }
    private:
        std::vector<double> k_;
        std::vector<double> kdot_;
    };

    // qwqfixme 椭圆滤波器的通带有点偏差
    //          偶数极点零点修改
    /**
     * @param amp_passband >0 dB
     * @param amp_stopband >0 dB
     * @ref Orfanidis lecture notes on Elliptic Filter Design.pdf
     */
    static void Elliptic(std::span<ZPK> ret, size_t num_filter, double db_passband, double db_stopband) {
        assert(ret.size() >= num_filter);

        auto eps_passband = std::sqrt(std::pow(10.0, db_passband / 10.0) - 1.0);
        auto eps_stopband = std::sqrt(std::pow(10.0, db_stopband / 10.0) - 1.0);
        auto k1 = eps_passband / eps_stopband;
        size_t N = 2 * num_filter;
        // ellipdeg k1 -> k
        double kdot = 0.0;
        {
            auto k1dot = EllipticHelper::Kdot(k1);
            size_t L = num_filter;
            EllipticHelper helper{k1dot};
            double f1 = std::pow(k1dot, N);
            std::complex<double> back{1.0, 0.0};
            for (size_t i = 1; i <= L; ++i) {
                auto ui = (2.0 * static_cast<double>(i) - 1.0) / static_cast<double>(N);
                back *= std::pow(helper.Sn(ui), 4.0);
            }
            kdot = f1 * std::real(back);
        }
        double k = EllipticHelper::Kdot(kdot);

        EllipticHelper helper{k};
        EllipticHelper helper1{k1};
        double const k1_complt_int = helper1.CompleteIntegral();
        auto const v0 = std::complex{0.0, -1.0} * helper1.ArcSn(std::complex{0.0, 1.0} / eps_passband) / (static_cast<double>(N) * k1_complt_int);
        for (size_t i = 0; i < num_filter; ++i) {
            auto& s = ret[i];
            auto ui = (2.0 * static_cast<double>(i + 1) - 1.0) / static_cast<double>(N);
            // zero
            auto epsi = helper.Cd(ui);
            s.z = std::complex<double>{0.0, 1.0} / (k * epsi);
            // pole
            s.p = std::complex{0.0, 1.0} * helper.Cd(ui - v0 * std::complex{0.0, 1.0});
            ret[i].k = std::norm(s.p) / std::norm(*s.z);
        }
        ret[0].k /= std::sqrt(1.0 + eps_passband * eps_passband);
    }

    // --------------------------------------------------------------------------------
    // 滤波器映射
    // --------------------------------------------------------------------------------
    /**
     * @param omega 模拟角频率
     */
    static void ProtyleToLowpass(std::span<ZPK> analog, size_t num_filter, double omega) {
        assert(analog.size() >= num_filter);

        for (size_t i = 0; i < num_filter; ++i) {
            const auto& s = analog[i];
            ZPK lps = analog[i];
            double gain = omega * omega;
            lps.p = ScaleComplex(s.p, omega);
            if (s.z) {
                lps.z = ScaleComplex(*s.z, omega);
                gain = 1.0;
            }
            analog[i] = lps;
            analog[i].k *= gain;
        }
    }

    /**
     * @param omega 模拟角频率
     */
    static void ProtyleToHighpass(std::span<ZPK> protyle, size_t num_filter, double omega) {
        assert(protyle.size() >= num_filter);

        for (size_t i = 0; i < num_filter; ++i) {
            const auto& s = protyle[i];
            ZPK hps = protyle[i];
            double gain = 1.0 / std::norm(s.p);
            hps.p = omega / s.p;
            if (s.z) {
                gain *= std::norm(*s.z);
                hps.z = omega / *s.z;
            }
            else {
                hps.z = 0;
            }
            protyle[i] = hps;
            protyle[i].k *= gain;
        }
    }

    /**
     * @note num_filter将会x2
     */
    static void ProtyleToBandpass(std::span<ZPK> protyle, size_t num_filter, double wo, double Q) {
        assert(protyle.size() >= num_filter * 2);

        double bw = wo / Q;
        for (size_t i = 0; i < num_filter; ++i) {
            // prototype -> lowpass at bw
            ZPK s = protyle[i];
            {
                auto const& ss = protyle[i];
                s.k = ss.k * bw * bw;
                s.p = ScaleComplex(ss.p, bw);
                if (ss.z) {
                    s.k = ss.k;
                    s.z = ScaleComplex(*ss.z, bw);
                }
            }
            // lowpass -> bandpass
            ZPK bp1;
            ZPK bp2;
            if (s.z) {
                auto p_delta = std::sqrt(s.p * s.p - 4.0 * wo * wo);
                auto z_delta = std::sqrt(*s.z * *s.z - 4.0 * wo * wo);
                bp1.p = ScaleComplex(s.p + p_delta, 0.5);
                bp2.p = ScaleComplex(s.p - p_delta, 0.5);
                bp1.z = ScaleComplex(*s.z + z_delta, 0.5);
                bp2.z = ScaleComplex(*s.z - z_delta, 0.5);
            }
            else {
                auto delta = std::sqrt(s.p * s.p - 4.0 * wo * wo);
                bp1.p = ScaleComplex(s.p + delta, 0.5);
                bp2.p = ScaleComplex(s.p - delta, 0.5);
                bp1.z = 0;
            }
            bp1.k = bp2.k = std::sqrt(s.k);
            protyle[i] = bp1;
            protyle[i + num_filter] = bp2;
        }
    }

    /**
     * @note num_filter将会x2
     */
    static void ProtyleToBandpass2(std::span<ZPK> protyle, size_t num_filter, double w1, double w2) {
        assert(protyle.size() >= num_filter * 2);

        double bw = w2 - w1;
        for (size_t i = 0; i < num_filter; ++i) {
            ZPK bp1;
            ZPK bp2;
            ZPK s = protyle[i];
            bp1.k = bp2.k = std::sqrt(s.k);;
            if (s.z) {
                auto p_delta = std::sqrt(s.p * s.p * bw * bw - 4.0 * w1 * w2);
                auto z_delta = std::sqrt(*s.z * *s.z * bw * bw - 4.0 * w1 * w2);
                bp1.p = ScaleComplex(s.p * bw + p_delta, 0.5);
                bp2.p = ScaleComplex(s.p * bw - p_delta, 0.5);
                bp1.z = ScaleComplex(*s.z * bw + z_delta, 0.5);
                bp2.z = ScaleComplex(*s.z * bw - z_delta, 0.5);
            }
            else {
                auto delta = std::sqrt(s.p * s.p * bw * bw - 4.0 * w1 * w2);
                bp1.p = ScaleComplex(s.p * bw + delta, 0.5);
                bp2.p = ScaleComplex(s.p * bw - delta, 0.5);
                bp1.z = 0;
                bp1.k *= bw;
                bp2.k *= bw;
            }
            protyle[i] = bp1;
            protyle[i + num_filter] = bp2;
        }
    }

    /**
     * @note num_filter将会x2
     */
    static void ProtyleToBandstop(std::span<ZPK> protyle, size_t num_filter, double wo, double Q) {
        assert(protyle.size() >= num_filter * 2);

        double bw = wo / Q;
        for (size_t i = 0; i < protyle.size(); ++i) {
            // prototype -> highpass at bw
            ZPK s = protyle[i];
            {
                auto const& ss = protyle[i];
                s.k /= std::norm(ss.p);
                s.p = bw / ss.p;
                if (ss.z) {
                    s.k *= std::norm(*ss.z);
                    s.z = bw / *ss.z;
                }
                else {
                    s.z = 0;
                }
            }
            // highpass -> bandstop
            ZPK bp1;
            ZPK bp2;
            bp1.k = bp2.k = std::sqrt(s.k);
            if (s.z) {
                auto p_delta = std::sqrt(s.p * s.p - 4.0 * wo * wo);
                auto z_delta = std::sqrt(*s.z * *s.z - 4.0 * wo * wo);
                bp1.p = ScaleComplex(s.p + p_delta, 0.5);
                bp2.p = ScaleComplex(s.p - p_delta, 0.5);
                bp1.z = ScaleComplex(*s.z + z_delta, 0.5);
                bp2.z = ScaleComplex(*s.z - z_delta, 0.5);
            }
            else {
                auto delta = std::sqrt(s.p * s.p - 4.0 * wo * wo);
                bp1.p = ScaleComplex(s.p + delta, 0.5);
                bp2.p = ScaleComplex(s.p - delta, 0.5);
                bp1.z = 0;
            }
            protyle[i] = bp1;
            protyle[i + num_filter] = bp2;
        }
    }

    /**
     * @note num_filter将会x2
     */
    static void ProtyleToBandstop2(std::span<ZPK> protyle, size_t num_filter, double w1, double w2) {
        assert(protyle.size() >= 2 * num_filter);

        double bw = w2 - w1;
        for (size_t i = 0; i < protyle.size(); ++i) {
            ZPK s = protyle[i];
            {
                auto const& ss = protyle[i];
                s.k /= std::norm(ss.p);
                s.p = bw / ss.p;
                if (ss.z) {
                    s.k *= std::norm(*ss.z);
                    s.z = bw / *ss.z;
                }
                else {
                    s.z = 0;
                }
            }

            ZPK bp1;
            ZPK bp2;
            bp1.k = bp2.k = std::sqrt(s.k);
            if (s.z) {
                auto p_delta = std::sqrt(s.p * s.p - 4.0 * w1 * w2);
                auto z_delta = std::sqrt(*s.z * *s.z - 4.0 * w1 * w2);
                bp1.p = ScaleComplex(s.p + p_delta, 0.5);
                bp2.p = ScaleComplex(s.p - p_delta, 0.5);
                bp1.z = ScaleComplex(*s.z + z_delta, 0.5);
                bp2.z = ScaleComplex(*s.z - z_delta, 0.5);
            }
            else {
                auto delta = std::sqrt(s.p * s.p - 4.0 * w1 * w2);
                bp1.p = ScaleComplex(s.p + delta, 0.5);
                bp2.p = ScaleComplex(s.p - delta, 0.5);
                bp1.z = 0;
            }
            protyle[i] = bp1;
            protyle[i + num_filter] = bp2;
        }
    }

    // --------------------------------------------------------------------------------
    // 离散化
    // --------------------------------------------------------------------------------
    static void Bilinear(std::span<ZPK> analog, double fs) {
        std::complex k = 2.0 * fs;
        for (size_t i = 0; i < analog.size(); ++i) {
            const ZPK& s = analog[i];
            ZPK z;
            if (s.z) {
                z.p = (k + s.p) / (k - s.p);
                z.z = (k + *s.z) / (k - *s.z);
                z.k = s.k * std::real((k - *s.z) * (k - std::conj(*s.z)) / (k - s.p) / (k - std::conj(s.p)));
            }
            else {
                z.p = (k + s.p) / (k - s.p);
                z.z = -1;
                z.k = s.k / std::real((k - s.p) * (k - std::conj(s.p)));
            }
            analog[i] = z;
        }
    }

    static void TfToBiquad(std::span<ZPK> digital, std::span<BiquadCoeff> biquad) {
        assert(biquad.size() >= digital.size());

        size_t num_filter = digital.size();
        for (size_t i = 0; i < num_filter; ++i) {
            const auto& z = digital[i];
            float k = static_cast<float>(z.k);
            float b0 = static_cast<float>(k);
            float b1 = static_cast<float>(-k * 2.0 * std::real(*z.z));
            float b2 = static_cast<float>(k * std::norm(*z.z));
            float a1 = static_cast<float>(-2.0 * std::real(z.p));
            float a2 = static_cast<float>(std::norm(z.p));
            
            auto& coeff = biquad[i];
            coeff.a1 = a1;
            coeff.a2 = a2;
            coeff.b0 = b0;
            coeff.b1 = b1;
            coeff.b2 = b2;
        }
    }

    /**
     * @return analog omega frequency!(rad/sec)
     */
    static double Digital2AnalogW(double freq, double fs) {
        return 2 * fs * std::tan(freq * pi / fs);
    }
};
}

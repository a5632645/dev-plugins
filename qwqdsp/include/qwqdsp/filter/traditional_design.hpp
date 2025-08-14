#pragma once
#include <optional>
#include <complex>
#include <vector>
#include <numbers>
#include "biquad.hpp"

namespace qwqdsp {
struct TraditionalDesign {
    static constexpr auto pi = std::numbers::pi;

    struct ZPK {
        std::optional<std::complex<double>> z; // 如果Null则在无穷远处
        std::complex<double> p;
    };

    struct FilterDesign {
        std::vector<ZPK> zpk;
        double k{1.0};

        FilterDesign(int num) {
            zpk.resize(num);
        }

        ZPK& operator[](size_t i) {
            return zpk[i];
        }

        const ZPK& operator[](size_t i) const {
            return zpk[i];
        }

        int size() const {
            return static_cast<int>(zpk.size());
        }
    };

    static std::complex<double> ScaleComplex(const std::complex<double>& a, double b) {
        return {a.real() * b, a.imag() * b};
    }

    // --------------------------------------------------------------------------------
    // 原型滤波器
    // --------------------------------------------------------------------------------
    static FilterDesign Butterworth(int num_filter) {
        FilterDesign ret{num_filter};

        int n = 2 * num_filter;
        int i = 0;
        for (int k = 1; k <= num_filter; ++k) {
            double phi = (2.0 * k - 1.0) * pi / (2.0 * n);
            ret[i].p = std::complex{-std::sin(phi), std::cos(phi)};
            ++i;
        }
        ret.k = 1.0;

        return ret;
    }

    static FilterDesign Chebyshev1(int num_filter, double ripple, bool even_pole_modify) {
        FilterDesign ret{num_filter};

        int n = 2 * num_filter;
        int i = 0;
        double eps = std::sqrt(std::pow(10.0, ripple / 10.0) - 1.0);
        double A = 1.0 / n * std::asinh(1.0 / eps);
        double k_re = std::sinh(A);
        double k_im = std::cosh(A);
        double gain = 1.0;
        double first_pole = std::cos(pi * (n - 1.0) / (2.0 * n));
        first_pole = first_pole * first_pole;
        for (int k = 1; k <= num_filter; ++k) {
            double phi = (2.0 * k - 1.0) * pi / (2.0 * n);
            if (even_pole_modify) {
                auto pole = std::complex{-std::sin(phi) * k_re, std::cos(phi) * k_im};
                ret[i].p = std::sqrt((pole * pole + first_pole) / (1.0 - first_pole));
            }
            else {
                ret[i].p = std::complex{-std::sin(phi) * k_re, std::cos(phi) * k_im};
            }
            gain *= std::norm(ret[i].p);
            ++i;
        }
        gain /= std::sqrt(1.0f + eps * eps);
        ret.k = gain;

        return ret;
    }

    // TODO: 偶数极点零点修改
    static FilterDesign Chebyshev2(int num_filter, double ripple) {
        FilterDesign ret{num_filter};

        int n = 2 * num_filter;
        int i = 0;
        double eps = 1.0 / std::sqrt(std::pow(10.0, -ripple / 10.0) - 1.0);
        double A = 1.0 / n * std::asinh(1.0 / eps);
        double scale = 1.0 / std::cosh(std::acosh(std::sqrt(std::pow(10.0, -ripple / 10.0) - 1.0)) / n);
        double k_re = std::sinh(A) * scale;
        double k_im = std::cosh(A) * scale;
        for (int k = 1; k <= num_filter; ++k) {
            double phi = (2.0 * k - 1.0) * pi / (2.0 * n);
            ret.zpk[i].z = 1.0 / std::complex{0.0, std::cos(phi) * scale};
            ret.zpk[i].p = 1.0 / std::complex{-std::sin(phi) * k_re, std::cos(phi) * k_im};
            ret.k *= std::norm(ret.zpk[i].p) / std::norm(*ret.zpk[i].z);
            ++i;
        }

        return ret;
    }

    struct EllipticHelper {
    public:
        static constexpr double kError = 1e-18;
        static constexpr bool kUseStd = true;

        EllipticHelper(double k0) {
            while (k0 > kError) {
                k_.push_back(k0);
                double kdot = std::sqrt(1.0 - k0 * k0);
                kdot_.push_back(kdot);
                k0 = (1.0 - kdot) / (1.0 + kdot);
            }
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
                    int m = GetM();
                    while (m > 0) {
                        km = km * (1 + k_[m]);
                        --m;
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
            auto cdm = std::cos(u * (pi / 2.0));
            int m = GetM();
            while (m > 0) {
                auto invcd = 1.0 / (1 + k_[m]) * (1.0 / cdm + k_[m] * cdm);
                cdm = 1.0 / invcd;
                --m;
            }
            return cdm;
        }

        std::complex<double> Sn(std::complex<double> u) {
            auto snm = std::sin(u * (pi / 2.0));
            int m = GetM();
            while (m > 0) {
                auto invsn = 1.0 / (1 + k_[m]) * (1.0 / snm + k_[m] * snm);
                snm = 1.0 / invsn;
                --m;
            }
            return snm;
        }

        std::complex<double> ArcSn(std::complex<double> sn0) {
            int m = GetM();
            for (int i = 1; i <= m; ++i) {
                sn0 = 2.0 * sn0 / ((1.0 + k_[i]) * std::sqrt(1.0 + std::sqrt(1.0 - k_[i - 1] * k_[i - 1] * sn0 * sn0)));
            }
            return std::asin(sn0) * (2.0 / pi);
        }

        int GetM() const {
            return k_.size() - 1;
        }

        static double Kdot(double k) {
            return std::sqrt(1.0 - k * k);
        }
    private:
        std::vector<double> k_;
        std::vector<double> kdot_;
    };

    // TODO: 椭圆滤波器的极点有点偏差
    //       偶数极点零点修改
    static FilterDesign Elliptic(int num_filter, double amp_passband, double amp_stopband) {
        FilterDesign ret{num_filter};

        auto eps_passband = std::sqrt(std::pow(10.0, amp_passband / 10.0) - 1.0);
        auto eps_stopband = std::sqrt(std::pow(10.0, amp_stopband / 10.0) - 1.0);
        auto k1 = eps_passband / eps_stopband;
        int N = 2 * num_filter;
        auto k1dot = EllipticHelper::Kdot(k1);
        // ellipdeg get kdot
        double kdot = 0.0;
        {
            int L = num_filter;
            EllipticHelper helper{k1dot};
            double f1 = std::pow(k1dot, N);
            std::complex<double> back{1.0, 0.0};
            for (int i = 1; i <= L; ++i) {
                auto ui = (2.0 * i - 1.0) / N;
                back *= std::pow(helper.Sn(ui), 4.0);
            }
            kdot = f1 * std::real(back);
        }
        double k = EllipticHelper::Kdot(kdot);

        EllipticHelper helper{k};
        EllipticHelper helper1{k1};
        double const k1_complt_int = helper1.CompleteIntegral();
        auto const v0 = std::complex{0.0, -1.0} * helper1.ArcSn(std::complex{0.0, 1.0} / eps_passband) / (N * k1_complt_int);
        double gain = 1.0;
        for (int i = 0; i < num_filter; ++i) {
            auto& s = ret[i];
            auto ui = (2.0 * (i + 1) - 1.0) / N;
            // zero
            auto epsi = helper.Cd(ui);
            s.z = std::complex<double>{0.0, 1.0} / (k * epsi);
            // pole
            s.p = std::complex{0.0, 1.0} * helper.Cd(ui - v0 * std::complex{0.0, 1.0});
            gain *= std::norm(s.p) / std::norm(*s.z);
        }
        gain /= std::sqrt(1.0 + eps_passband * eps_passband);
        ret.k = gain;

        return ret;
    }

    // --------------------------------------------------------------------------------
    // 滤波器映射
    // --------------------------------------------------------------------------------
    static FilterDesign ProtyleToLowpass(const FilterDesign& analog, double omega) {
        FilterDesign ret{analog.size()};
        ret.k = analog.k;
        for (int i = 0; i < analog.size(); ++i) {
            const auto& s = analog[i];
            auto& lps = ret[i];
            double gain = omega * omega;
            lps.p = ScaleComplex(s.p, omega);
            if (s.z) {
                lps.z = ScaleComplex(*s.z, omega);
                gain = 1.0;
            }
            ret.k *= gain;
        }
        return ret;
    }

    static FilterDesign ProtyleToHighpass(const FilterDesign& protyle, double omega) {
        FilterDesign ret{protyle.size()};
        ret.k = protyle.k;
        for (int i = 0; i < protyle.size(); ++i) {
            const auto& s = protyle[i];
            auto& hps = ret[i];
            double gain = 1.0 / std::norm(s.p);
            hps.p = omega / s.p;
            if (s.z) {
                gain *= std::norm(*s.z);
                hps.z = omega / *s.z;
            }
            else {
                hps.z = 0;
            }
            ret.k *= gain;
        }
        return ret;
    }

    FilterDesign ProtyleToBandpass(const FilterDesign& protyle, double wo, double Q) {
        FilterDesign ret{protyle.size() * 2};
        ret.k = protyle.k;
        
        double bw = wo / Q;
        for (int i = 0; i < protyle.size(); ++i) {
            // prototype -> lowpass at bw
            ZPK s;
            double gain = 1.0;
            {
                auto const& ss = protyle[i];
                gain = bw * bw;
                s.p = ScaleComplex(ss.p, bw);
                if (ss.z) {
                    gain = 1.0;
                    s.z = ScaleComplex(*ss.z, bw);
                }
            }
            // lowpass -> bandpass
            auto& bp1 = ret[2 * i];
            auto& bp2 = ret[2 * i + 1];
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
            ret.k *= gain;
        }
        return ret;
    }

    FilterDesign ProtyleToBandpass2(const FilterDesign& protyle, double w1, double w2) {
        FilterDesign ret{protyle.size() * 2};
        ret.k = protyle.k;

        double bw = w2 - w1;
        for (int i = 0; i < protyle.size(); ++i) {
            auto& bp1 = ret[2 * i];
            auto& bp2 = ret[2 * i + 1];
            const auto& s = protyle[i];
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
                ret.k *= (bw * bw);
            }
        }
        return ret;
    }

    FilterDesign ProtyleToBandstop(const FilterDesign& protyle, double wo, double Q) {
        FilterDesign ret{protyle.size() * 2};
        ret.k = protyle.k;

        double bw = wo / Q;
        for (int i = 0; i < protyle.size(); ++i) {
            // prototype -> highpass at bw
            ZPK s;
            double gain = 1.0;
            {
                auto const& ss = protyle[i];
                gain /= std::norm(ss.p);
                s.p = bw / ss.p;
                if (ss.z) {
                    gain *= std::norm(*ss.z);
                    s.z = bw / *ss.z;
                }
                else {
                    s.z = 0;
                }
            }
            // highpass -> bandstop
            auto& bp1 = ret[2 * i];
            auto& bp2 = ret[2 * i + 1];
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
            ret.k *= gain;
        }
        return ret;
    }

    FilterDesign ProtyleToBandstop2(const FilterDesign& protyle, double w1, double w2) {
        FilterDesign ret{protyle.size() * 2};
        ret.k = protyle.k;
        
        double bw = w2 - w1;
        for (int i = 0; i < protyle.size(); ++i) {
            ZPK s;
            double gain = 1.0;
            {
                auto const& ss = protyle[i];
                gain /= std::norm(ss.p);
                s.p = bw / ss.p;
                if (ss.z) {
                    gain *= std::norm(*ss.z);
                    s.z = bw / *ss.z;
                }
                else {
                    s.z = 0;
                }
            }
            auto& bp1 = ret[2 * i];
            auto& bp2 = ret[2 * i + 1];
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
            ret.k *= gain;
        }
        return ret;
    }

    // --------------------------------------------------------------------------------
    // 离散化
    // --------------------------------------------------------------------------------
    static FilterDesign Bilinear(const FilterDesign& analog, double fs) {
        FilterDesign ret{analog.size()};
        ret.k = analog.k;

        std::complex k = 2.0 * fs;
        int num_filter = static_cast<int>(analog.size());
        for (int i = 0; i < num_filter; ++i) {
            const ZPK& s = analog[i];
            ZPK& z = ret[i];
            double gain = 1.0;
            if (s.z) {
                z.p = (k + s.p) / (k - s.p);
                z.z = (k + *s.z) / (k - *s.z);
                gain = std::real((k - *s.z) * (k - std::conj(*s.z)) / (k - s.p) / (k - std::conj(s.p)));
            }
            else {
                z.p = (k + s.p) / (k - s.p);
                z.z = -1;
                gain = 1.0 / std::real((k - s.p) * (k - std::conj(s.p)));
            }
            ret.k *= gain;
        }

        return ret;
    }

    static std::vector<Biquad> TfToBiquad(const FilterDesign& digital) {
        std::vector<Biquad> ret{digital.zpk.size()};

        size_t num_filter = digital.size();
        double k = std::pow(digital.k, 1.0 / num_filter);
        for (size_t i = 0; i < num_filter; ++i) {
            const auto& z = digital[i];
            auto& biquad = ret[i];
            float b0 = k;
            float b1 = -k * 2.0 * std::real(*z.z);
            float b2 = k * std::norm(*z.z);
            float a1 = -2.0 * std::real(z.p);
            float a2 = std::norm(z.p);
            biquad.Set(b0, b1, b2, a1, a2);
        }

        return ret;
    }

    /**
     * @return analog omega frequency!(rad/sec)
     */
    static double Prewarp(double freq, double fs) {
        return 2 * fs * std::tan(freq * pi / fs);
    }
};
}
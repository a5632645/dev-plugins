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

    static FilterDesign Chebyshev1(int num_filter, double ripple) {
        FilterDesign ret{num_filter};

        int n = 2 * num_filter;
        int i = 0;
        double eps = std::sqrt(std::pow(10.0, ripple / 10.0) - 1.0);
        double A = 1.0 / n * std::asinh(1.0 / eps);
        double k_re = std::sinh(A);
        double k_im = std::cosh(A);
        double gain = 1.0;
        for (int k = 1; k <= num_filter; ++k) {
            double phi = (2.0 * k - 1.0) * pi / (2.0 * n);
            ret[i].p = std::complex{-std::sin(phi) * k_re, std::cos(phi) * k_im};
            gain *= std::norm(ret[i].p);
            ++i;
        }
        // 切比雪夫I型在DC处的增益, s = 0, 给到第一个滤波器
        gain /= std::sqrt(1.0f + eps * eps);
        ret.k = gain;

        return ret;
    }

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

    // TODO: fix frequency error
    static FilterDesign ProtyleToBandpass(const FilterDesign& protyle, double w, double Q) {
        double bw = w / Q;
        FilterDesign ret{protyle.size() * 2};
        ret.k = protyle.k;
        for (int i = 0; i < protyle.size(); ++i) {
            ZPK s;
            double gain;
            {
                auto const& ss = protyle[i];
                gain = bw * bw / 4;
                s.p = ScaleComplex(ss.p, bw / 2);
                if (ss.z) {
                    gain = 1.0;
                    s.z = ScaleComplex(*ss.z, bw / 2);
                }
            }
            auto& bp1 = ret[2 * i];
            auto& bp2 = ret[2 * i + 1];
            if (s.z) {
                auto p_delta = std::sqrt(s.p * s.p - 4.0 * w * w);
                auto z_delta = std::sqrt(*s.z * *s.z - 4.0 * w * w);
                bp1.p = ScaleComplex(s.p + p_delta, 0.5);
                bp2.p = ScaleComplex(s.p - p_delta, 0.5);
                bp1.z = ScaleComplex(*s.z + z_delta, 0.5);
                bp2.z = ScaleComplex(*s.z - z_delta, 0.5);
            }
            else {
                auto delta = std::sqrt(s.p * s.p - 4.0 * w * w);
                bp1.p = ScaleComplex(s.p + delta, 0.5);
                bp2.p = ScaleComplex(s.p - delta, 0.5);
                bp1.z = 0;
            }
            ret.k *= gain;
        }
        return ret;
    }

    // TODO: fix frequency error
    static FilterDesign ProtyleToBandstop(const FilterDesign& protyle, double w, double Q) {
        double bw = w / Q;
        double w0 = w - bw / 2;
        double w1 = w + bw / 2;
        double wo = std::sqrt(w0 * w1);
        FilterDesign ret{protyle.size() * 2};
        ret.k = protyle.k;
        for (int i = 0; i < protyle.size(); ++i) {
            ZPK s;
            double gain = 1.0;
            {
                auto const& ss = protyle[i];
                gain /= std::norm(ss.p);
                s.p = bw * 0.5 / ss.p;
                if (ss.z) {
                    gain *= std::norm(*ss.z);
                    s.z = bw * 0.5 / *ss.z;
                }
                else {
                    s.z = 0;
                }
            }
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
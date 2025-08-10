#pragma once
#include <cmath>
#include <complex>
#include <cstddef>
#include <array>
#include <optional>
#include <numbers>
#include <vector>

namespace qwqdsp {
class Biquad {
public:
    float ProcessSingle(float x) {
        auto output = x * b0_ + latch1_;
        latch1_ = x * b1_ - output * a1_ + latch2_;
        latch2_ = x * b2_ - output * a2_;
        return output;
    }

    /*
    *  b0 + b1 * z^-1 + b2 * z^-2
    * ----------------------------
    *   1 + a1 * z^-1 + a2 * z^-2
    */
    void FromZ(float b0, float b1, float b2, float a1, float a2) {
        b0_ = b0;
        b1_ = b1;
        b2_ = b2;
        a1_ = a1;
        a2_ = a2;
    }

    /*
    *  b0 * z^2 + b1 * z + b2
    * ------------------------
    *   z^2 + a1 * z + a2
    */
    void FromInvZ(float b0, float b1, float b2, float a1, float a2) {
        b0_ = b0;
        b1_ = b1;
        b2_ = b2;
        a1_ = a1;
        a2_ = a2;
    }

    void Copy(const Biquad& other) {
        b0_ = other.b0_;
        b1_ = other.b1_;
        b2_ = other.b2_;
        a1_ = other.a1_;
        a2_ = other.a2_;
    }
private:
    float b0_{};
    float b1_{};
    float b2_{};
    float a1_{};
    float a2_{};
    float latch1_{};
    float latch2_{};
};

namespace internal {
struct ZPK {
    double k;
    std::optional<std::complex<double>> z; // 如果Null则在无穷远处
    std::complex<double> p;

    std::complex<double> GetAnalogResponce(double omega) const {
        auto s = std::complex{0.0, omega};
        if (z) {
            auto up = (s - *z) * (s - std::conj(*z));
            auto down = (s - p) * (s - std::conj(p));
            return up * k / down;
        }
        else {
            auto up = k;
            auto down = (s - p) * (s - std::conj(p));
            return up / down;
        }
    }

    double GetQ() const {
        return std::abs(p) / (2.0 * std::real(p));
    }
};

constexpr auto pi = std::numbers::pi;

static std::vector<ZPK> Chebyshev2(int num_filter, double ripple) {
    std::vector<ZPK> ret{static_cast<size_t>(num_filter)};

    int n = 2 * num_filter;
    int i = 0;
    double eps = 1.0 / std::sqrt(std::pow(10.0, -ripple / 10.0) - 1.0);
    double A = 1.0 / n * std::asinh(1.0 / eps);
    double k_re = std::sinh(A);
    double k_im = std::cosh(A);
    for (int k = 1; k <= num_filter; ++k) {
        double phi = (2.0 * k - 1.0) * pi / (2.0 * n);
        ret[i].z = 1.0 / std::complex{0.0, std::cos(phi)};
        ret[i].p = 1.0 / std::complex{-std::sin(phi) * k_re, std::cos(phi) * k_im};
        ret[i].k = std::norm(ret[i].p) / std::norm(*ret[i].z);
        ++i;
    }

    return ret;
}

static std::complex<double> ScaleComplex(const std::complex<double>& a, double b) {
    return {a.real() * b, a.imag() * b};
}

static std::vector<ZPK> ProtyleToBandpass(const std::vector<ZPK>& protyle, double fcutoff, double Q) {
    double w = fcutoff * 2 * pi;
    double bw = w / Q;
    double w0 = w - bw / 2;
    double w1 = w + bw / 2;
    double wo = std::sqrt(w0 * w1);
    std::vector<ZPK> ret{protyle.size() * 2};
    for (size_t i = 0; i < protyle.size(); ++i) {
        ZPK s;
        {
            auto const& ss = protyle[i];
            s.k = ss.k * bw * bw / 4;
            s.p = ScaleComplex(ss.p, bw / 2);
            if (ss.z) {
                s.k = ss.k;
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
            bp1.k = std::sqrt(s.k);
            bp2.k = std::sqrt(s.k);
        }
        else {
            auto delta = std::sqrt(s.p * s.p - 4.0 * w * w);
            bp1.p = ScaleComplex(s.p + delta, 0.5);
            bp2.p = ScaleComplex(s.p - delta, 0.5);
            bp1.z = 0;
            bp1.k = std::sqrt(s.k);
            bp2.k = std::sqrt(s.k);
        }
    }
    return ret;
}

static std::vector<ZPK> ProtyleToBandpass2(const std::vector<ZPK>& protyle, double w0, double w1) {
    double w = std::sqrt(w0 * w1);
    double bw = w1 - w0;
    std::vector<ZPK> ret{protyle.size() * 2};
    for (size_t i = 0; i < protyle.size(); ++i) {
        ZPK s;
        {
            auto const& ss = protyle[i];
            s.k = ss.k * bw * bw / 4;
            s.p = ScaleComplex(ss.p, bw / 2);
            if (ss.z) {
                s.k = ss.k;
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
            bp1.k = std::sqrt(s.k);
            bp2.k = std::sqrt(s.k);
        }
        else {
            auto delta = std::sqrt(s.p * s.p - 4.0 * w * w);
            bp1.p = ScaleComplex(s.p + delta, 0.5);
            bp2.p = ScaleComplex(s.p - delta, 0.5);
            bp1.z = 0;
            bp1.k = std::sqrt(s.k);
            bp2.k = std::sqrt(s.k);
        }
    }
    return ret;
}

static std::vector<ZPK> Bilinear(const std::vector<ZPK>& analog, double fs) {
    std::vector<ZPK> ret{analog.size()};

    std::complex k = 2.0 * fs;
    int num_filter = static_cast<int>(analog.size());
    for (int i = 0; i < num_filter; ++i) {
        const ZPK& s = analog[i];
        ZPK& z = ret[i];
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
    }

    return ret;
}

static std::vector<Biquad> TfToBiquad(const std::vector<ZPK>& digital) {
    std::vector<Biquad> ret{digital.size()};

    size_t num_filter = digital.size();
    for (size_t i = 0; i < num_filter; ++i) {
        const auto& z = digital[i];
        auto& biquad = ret[i];
        float b0 = z.k;
        float b1 = -z.k * 2.0 * std::real(*z.z);
        float b2 = z.k * std::norm(*z.z);
        float a1 = -2.0 * std::real(z.p);
        float a2 = std::norm(z.p);
        biquad.FromInvZ(b0, b1, b2, a1, a2);
    }

    return ret;
}
}

class Cheb2Bandpass {
public:
    static constexpr size_t kNumFilter = 8;

    void MakeBandpass(float w1, float w2, float fs) {
        auto zpk = internal::Chebyshev2(kNumFilter, -100.0);
        zpk = internal::ProtyleToBandpass2(zpk, w1, w2);
        zpk = internal::Bilinear(zpk, fs);
        auto ffs = internal::TfToBiquad(zpk);
        for (size_t i = 0; i < kNumFilter; ++i) {
            filters_[i].Copy(ffs[i]);
        }
    }

    void MakeBandpassQ(float w, float Q, float fs) {
        auto zpk = internal::Chebyshev2(kNumFilter, -100.0);
        zpk = internal::ProtyleToBandpass(zpk, w, Q);
        zpk = internal::Bilinear(zpk, fs);
        auto ffs = internal::TfToBiquad(zpk);
        for (size_t i = 0; i < kNumFilter; ++i) {
            filters_[i].Copy(ffs[i]);
        }
    }
private:
    std::array<Biquad, kNumFilter> filters_;
};
}
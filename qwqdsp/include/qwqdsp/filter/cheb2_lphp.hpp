#pragma once
#include <cmath>
#include <complex>
#include <cstddef>
#include <array>
#include <optional>
#include <numbers>
#include <vector>

namespace qwqdsp {
struct Biquad {
    float Tick(float x) {
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

    void Reset() {
        latch1_ = 0;
        latch2_ = 0;
    }

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

constexpr auto pi = std::numbers::pi;

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

static std::complex<double> ScaleComplex(const std::complex<double>& a, double b) {
    return {a.real() * b, a.imag() * b};
}

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
        biquad.FromInvZ(b0, b1, b2, a1, a2);
    }

    return ret;
}

static double Prewarp(double w, double fs) {
    return 2 * fs * std::tan(w / (2 * fs));
}
}

// TODO: fix frequency not correct
template<size_t kNumFilter>
class Cheb2LowHighpass {
public:
    void Init(float fs) {
        fs_ = fs;
    }

    float Tick(float x) {
        for (auto& f : filters_) {
            x = f.Tick(x);
        }
        return x;
    }

    void MakeLowpass(float freq, float ripple) {
        auto w = freq * std::numbers::pi * 2;
        w = internal::Prewarp(w, fs_);
        auto zpk = internal::Chebyshev2(kNumFilter, ripple);
        zpk = internal::ProtyleToLowpass(zpk, w);
        zpk = internal::Bilinear(zpk, fs_);
        auto ffs = internal::TfToBiquad(zpk);
        for (size_t i = 0; i < kNumFilter; ++i) {
            filters_[i].Copy(ffs[i]);
        }
    }

    void MakeHighpass(float freq, float ripple) {
        auto w = freq * std::numbers::pi * 2;
        w = internal::Prewarp(w, fs_);
        auto zpk = internal::Chebyshev2(kNumFilter, ripple);
        zpk = internal::ProtyleToHighpass(zpk, w);
        zpk = internal::Bilinear(zpk, fs_);
        auto ffs = internal::TfToBiquad(zpk);
        for (size_t i = 0; i < kNumFilter; ++i) {
            filters_[i].Copy(ffs[i]);
        }
    }

    void Reset() {
        for (auto& f : filters_) {
            f.Reset();
        }
    }
private:
    float fs_{};
    std::array<Biquad, kNumFilter> filters_;
};
}
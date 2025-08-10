#pragma once
#include "traditional_design.hpp"

namespace qwqdsp {
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
        w = TraditionalDesign::Prewarp(w, fs_);
        auto zpk = TraditionalDesign::Chebyshev2(kNumFilter, ripple);
        zpk = TraditionalDesign::ProtyleToLowpass(zpk, w);
        zpk = TraditionalDesign::Bilinear(zpk, fs_);
        auto ffs = TraditionalDesign::TfToBiquad(zpk);
        for (size_t i = 0; i < kNumFilter; ++i) {
            filters_[i].Copy(ffs[i]);
        }
    }

    void MakeHighpass(float freq, float ripple) {
        auto w = freq * std::numbers::pi * 2;
        w = TraditionalDesign::Prewarp(w, fs_);
        auto zpk = TraditionalDesign::Chebyshev2(kNumFilter, ripple);
        zpk = TraditionalDesign::ProtyleToHighpass(zpk, w);
        zpk = TraditionalDesign::Bilinear(zpk, fs_);
        auto ffs = TraditionalDesign::TfToBiquad(zpk);
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
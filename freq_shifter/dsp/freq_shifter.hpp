#pragma once
#include <span>
#include "qwqdsp/iir_hilbert.hpp"
#include "qwqdsp/iir_cpx_hilbert.hpp"
#include "qwqdsp/osciilor/vic_sine_osc.hpp"
// #include "qwqdsp/filter/cheb2_lphp.hpp"
// #include "qwqdsp/filter/complex_halfband.hpp"

namespace dsp {
class FreqShifter {
public:
    void Init(float fs) {
        fs_ = fs;
        osc_.Reset(0.5f);
        // cheb2_.Init(fs);
    }

    void Process(std::span<float> left, std::span<float> right) {
        for (auto& s : left) {
            // auto x = cheb2_.Tick(s);
            // s = x;
            auto a = hilbert_.Tick(s);
            osc_.Tick();
            auto b = osc_.GetCpx();
            auto c = a * b;
            c = filter_.Tick(c);
            s = c.real();
        }
        std::copy(left.begin(), left.end(), right.begin());
    }

    void SetShift(float freq) {
        osc_.SetFreq(freq, fs_);
        if (freq * freq_ < 0) {
            // cheb2_.Reset();
        }
        freq_ = freq;
        if (freq > 0) {
            // cheb2_.MakeLowpass(std::min(22000.0f, fs_ / 2 - freq), -100.0f);
        }
        else {
            // cheb2_.MakeHighpass(std::max(20.0f, -freq), -100.0f);
        }
    }
private:
    qwqdsp::VicSineOsc osc_;
    qwqdsp::IIRHilbertDeeper<float> hilbert_;
    qwqdsp::IIRHilbertDeeperCpx<float> filter_;
    // qwqdsp::Cheb2LowHighpass<8> cheb2_;
    // qwqdsp::ComplexHalfband<16> filter_;
    float fs_;
    float freq_;
};
}
#pragma once
#include <span>
#include "qwqdsp/iir_hilbert4.hpp"
#include "qwqdsp/osciilor/mcf_sine_osc.hpp"

namespace dsp {
class FreqShifter {
public:
    void Init(float fs) {
        fs_ = fs;
        osc_.Reset(0.0f, fs, 0.0f);
    }

    void Process(std::span<float> left, std::span<float> right) {
        for (auto& s : left) {
            auto a = hilbert_.Tick(s);
            osc_.Tick();
            auto b = osc_.GetCpx();
            auto c = a * b;
            s = c.real();
        }
        std::copy(left.begin(), left.end(), right.begin());
    }

    void SetShift(float freq) {
        osc_.SetFreq(freq, fs_);
    }
private:
    qwqdsp::FullMCFSineOsc osc_;
    qwqdsp::IIRHilbertFull<16> hilbert_;
    float fs_;
};
}
#pragma once
#include <qwqdsp/simd_element/one_pole_tpt_shelf.hpp>
#include <qwqdsp/convert.hpp>

namespace green_vocoder::dsp {
class TiltFilter {
public:
    void Reset() noexcept {
        state1_.Reset();
        state2_.Reset();
        state3_.Reset();
        state4_.Reset();
    }

    void SetTilt(float fs, float db) noexcept {
        float g = std::pow(10.0f, db / 40.0f);
        state1_.Set(qwqdsp::convert::Freq2W(18.0f, fs), g);
        state2_.Set(qwqdsp::convert::Freq2W(180.0f, fs), g);
        state3_.Set(qwqdsp::convert::Freq2W(1800.0f, fs), g);
        state4_.Set(qwqdsp::convert::Freq2W(18000.0f, fs), g);
    }

    qwqdsp_simd_element::PackFloat<2> Tick(qwqdsp_simd_element::PackFloat<2> x) noexcept {
        x = state1_.TickTiltshelf(x);
        x = state2_.TickTiltshelf(x);
        x = state3_.TickTiltshelf(x);
        x = state4_.TickTiltshelf(x);
        return x;
    }
private:
    qwqdsp_simd_element::OnepoleTPTShelf<2> state1_;
    qwqdsp_simd_element::OnepoleTPTShelf<2> state2_;
    qwqdsp_simd_element::OnepoleTPTShelf<2> state3_;
    qwqdsp_simd_element::OnepoleTPTShelf<2> state4_;
};
}

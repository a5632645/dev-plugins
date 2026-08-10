#pragma once
#include <qwqdsp/convert.hpp>
#include <qwqdsp/simd_element/one_pole_tpt_shelf.hpp>

namespace green_vocoder::dsp {
class TiltFilter {
public:
    struct Params {
        float db{10.0f};
    };

    void Init(float fs) noexcept {
        sample_rate_ = fs;
    }

    void Reset() noexcept {
        state1_.Reset();
        state2_.Reset();
        state3_.Reset();
        state4_.Reset();
    }

    void SetParam(const Params& p) noexcept {
        float g = std::pow(10.0f, p.db / 40.0f);
        state1_.Set(qwqdsp::convert::Freq2W(18.0f, sample_rate_), g);
        state2_.Set(qwqdsp::convert::Freq2W(180.0f, sample_rate_), g);
        state3_.Set(qwqdsp::convert::Freq2W(1800.0f, sample_rate_), g);
        state4_.Set(qwqdsp::convert::Freq2W(18000.0f, sample_rate_), g);
    }

    qwqdsp_simd_element::PackFloat<2> Tick(qwqdsp_simd_element::PackFloat<2> x) noexcept {
        x = state1_.TickTiltshelf(x);
        x = state2_.TickTiltshelf(x);
        x = state3_.TickTiltshelf(x);
        x = state4_.TickTiltshelf(x);
        return x;
    }
private:
    float sample_rate_{};
    qwqdsp_simd_element::OnepoleTPTShelf<2> state1_;
    qwqdsp_simd_element::OnepoleTPTShelf<2> state2_;
    qwqdsp_simd_element::OnepoleTPTShelf<2> state3_;
    qwqdsp_simd_element::OnepoleTPTShelf<2> state4_;
};
} // namespace green_vocoder::dsp

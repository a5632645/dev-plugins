#pragma once
#include <qwqdsp/simd_element/simd_pack.hpp>
#include <qwqdsp/misc/smoother.hpp>

namespace green_vocoder::dsp{

template<size_t N>
class ExpSmoother {
public:
    qwqdsp_simd_element::PackFloat<N> Process(qwqdsp_simd_element::PackFloatCRef<N> in) {
        auto mask = in > lag_;
        auto coeff = qwqdsp_simd_element::PackOps::Select(mask, biggerCoeff_, smallerCoeff_);
        lag_ = lag_ * coeff + (1 - coeff) * in;
        return lag_;
    }

    void SetAttackTime(float ms, float fs) {
        biggerCoeff_.Broadcast(qwqdsp_misc::ExpSmoother::ComputeSmoothFactor(ms, fs, 3));
    }

    void SetReleaseTime(float ms, float fs) {
        smallerCoeff_.Broadcast(qwqdsp_misc::ExpSmoother::ComputeSmoothFactor(ms, fs));
    }
private:
    qwqdsp_simd_element::PackFloat<N> lag_{};
    qwqdsp_simd_element::PackFloat<N> biggerCoeff_{};
    qwqdsp_simd_element::PackFloat<N> smallerCoeff_{};
};

}

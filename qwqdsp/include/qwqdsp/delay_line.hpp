#pragma once
#include <assert.h>
#include <cstddef>
#include <vector>
#include <cmath>
#include "interpolation.hpp"

namespace qwqdsp {
template<Interpolation::Type INTERPOLATION_TYPE = Interpolation::Type::Lagrange3rd>
class DelayLine {
public:
    void Init(float max_ms, float fs) {
        float d = max_ms * fs / 1000.0f;
        size_t i = static_cast<size_t>(std::ceil(d) + 2.0f);
        size_t a = 1;
        while (a < i) {
            a *= 2;
        }
        buffer_.resize(a);
        mask_ = a - 1;
    }

    void Push(float x) {
        buffer_[wpos_++] = x;
        wpos_ &= mask_;
    }

    float GetAfterPush(float delay_samples) {
        return Get(delay_samples);
    }

    float GetBeforePush(float delay_samples) {
        return Get(delay_samples - 1);
    }

    float GetBeforePush(int delay_samples) {
        int rpos = wpos_ - delay_samples - 1;
        int irpos = static_cast<int>(rpos) & mask_;
        return buffer_[irpos];
    }

    void Reset() {
        std::fill(buffer_.begin(), buffer_.end(), float{});
    }
private:
    float Get(float delay) {
        float rpos = wpos_ + buffer_.size() - delay;
        int irpos = static_cast<int>(rpos) & mask_;
        int inext1 = (irpos + 1) & mask_;
        int inext2 = (irpos + 2) & mask_;
        [[maybe_unused]] int inext3 = (irpos + 3) & mask_;
        [[maybe_unused]] int iprev1 = (irpos - 1) & mask_;
        float t = rpos - static_cast<int>(rpos);
        if constexpr (INTERPOLATION_TYPE == Interpolation::Type::None) {
            return buffer_[irpos];
        }
        else if constexpr (INTERPOLATION_TYPE == Interpolation::Type::Lagrange3rd) {
            return Interpolation::Lagrange3rd(buffer_[irpos], buffer_[inext1], buffer_[inext2], buffer_[inext3], t);
        }
        else if constexpr (INTERPOLATION_TYPE == Interpolation::Type::Linear) {
            return Interpolation::Linear(buffer_[irpos], buffer_[inext1], t);
        }
        else if constexpr (INTERPOLATION_TYPE == Interpolation::Type::PCHIP) {
            return Interpolation::PCHIP(buffer_[iprev1], buffer_[irpos], buffer_[inext1], buffer_[inext2], t);
        }
        else if constexpr (INTERPOLATION_TYPE == Interpolation::Type::Spline) {
            return Interpolation::Spline(buffer_[iprev1], buffer_[irpos], buffer_[inext1], buffer_[inext2], t);
        }
        else if constexpr (INTERPOLATION_TYPE == Interpolation::Type::CatmullRomSpline) {
            return Interpolation::CatmullRomSpline(buffer_[iprev1], buffer_[irpos], buffer_[inext1], buffer_[inext2], t);
        }
    }

    std::vector<float> buffer_;
    size_t wpos_{};
    size_t mask_{};
};
}
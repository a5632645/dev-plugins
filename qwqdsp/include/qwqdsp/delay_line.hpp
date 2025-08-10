#pragma once
#include <assert.h>
#include <cstddef>
#include <vector>
#include <cmath>
#include "interpolation.hpp"

namespace qwqdsp {
template<bool INTERPOLATION = true>
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
        if constexpr (INTERPOLATION) {
            int inext1 = (irpos + 1) & mask_;
            int inext2 = (irpos + 2) & mask_;
            int inext3 = (irpos + 3) & mask_;
            int iprev1 = (irpos - 1) & mask_;
            float t = rpos - static_cast<int>(rpos);
            return Interpolation::CatmullRomSpline(buffer_[iprev1], buffer_[irpos], buffer_[inext1], buffer_[inext2], t);
        }
        else {
            return buffer_[irpos];
        }
    }

    std::vector<float> buffer_;
    size_t wpos_{};
    size_t mask_{};
};
}
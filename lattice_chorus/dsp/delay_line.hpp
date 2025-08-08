#pragma once
#include <assert.h>
#include <cstddef>
#include <type_traits>
#include <vector>
#include <cmath>

namespace dsp {
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
        float rpos = wpos_ + buffer_.size() - delay_samples;
        int irpos = static_cast<int>(rpos) & mask_;
        if constexpr (INTERPOLATION) {
            int inext1 = (irpos + 1) & mask_;
            int inext2 = (irpos + 2) & mask_;
            int inext3 = (irpos + 3) & mask_;
            float t = rpos - static_cast<int>(rpos);
    
            float p1 = buffer_[irpos];
            float p2 = buffer_[inext1];
            float p3 = buffer_[inext2];
            float p4 = buffer_[inext3];
            
            auto d1 = t - 1.f;
            auto d2 = t - 2.f;
            auto d3 = t - 3.f;

            auto c1 = -d1 * d2 * d3 / 6.f;
            auto c2 = d2 * d3 * 0.5f;
            auto c3 = -d1 * d3 * 0.5f;
            auto c4 = d1 * d2 / 6.f;

            return p1 * c1 + t * (p2 * c2 + p3 * c3 + p4 * c4);
        }
        else {
            return buffer_[irpos];
        }
    }

    float GetBeforePush(float delay_samples) {
        float rpos = wpos_ + buffer_.size() - delay_samples - 1;
        int irpos = static_cast<int>(rpos) & mask_;
        if constexpr (INTERPOLATION) {
            int inext1 = (irpos + 1) & mask_;
            int inext2 = (irpos + 2) & mask_;
            int inext3 = (irpos + 3) & mask_;
            float t = rpos - static_cast<int>(rpos);
    
            float p1 = buffer_[irpos];
            float p2 = buffer_[inext1];
            float p3 = buffer_[inext2];
            float p4 = buffer_[inext3];
            
            auto d1 = t - 1.f;
            auto d2 = t - 2.f;
            auto d3 = t - 3.f;

            auto c1 = -d1 * d2 * d3 / 6.f;
            auto c2 = d2 * d3 * 0.5f;
            auto c3 = -d1 * d3 * 0.5f;
            auto c4 = d1 * d2 / 6.f;

            return p1 * c1 + t * (p2 * c2 + p3 * c3 + p4 * c4);
        }
        else {
            return buffer_[irpos];
        }
    }

    float GetBeforePush(int delay_samples) {
        int rpos = wpos_ - delay_samples - 1;
        int irpos = static_cast<int>(rpos) & mask_;
        return buffer_[irpos];
    }
private:
    std::vector<float> buffer_;
    size_t wpos_{};
    size_t mask_{};
};
}
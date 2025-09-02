#pragma once
#include <assert.h>
#include <concepts>
#include <cstddef>
#include <vector>
#include <cmath>
#include "qwqdsp/interpolation.hpp"

namespace qwqdsp::fx {
enum class DelayLineInterp {
    None,
    Lagrange3rd,
    PCHIP,
    Linear,
};

template<DelayLineInterp INTERPOLATION_TYPE = DelayLineInterp::Lagrange3rd>
class DelayLine {
public:
    void Init(float max_ms, float fs) {
        float d = max_ms * fs / 1000.0f;
        size_t i = static_cast<size_t>(std::ceil(d) + 4.0f);
        Init(i);
    }

    void Init(size_t max_samples) {
        size_t a = 1;
        while (a < max_samples) {
            a *= 2;
        }
        if (buffer_.size() < a) {
            buffer_.resize(a);
        }
        mask_ = a - 1;
        Reset();
    }

    void Reset() noexcept {
        wpos_ = 0;
        std::fill(buffer_.begin(), buffer_.end(), 0.0f);
    }

    void Push(float x) noexcept {
        buffer_[wpos_++] = x;
        wpos_ &= mask_;
    }

    float GetAfterPush(float delay_samples) noexcept {
        return Get(delay_samples + 1);
    }

    /**
     * @param delay_samples 此处不能小于1，否则为非因果滤波器（或者被绕回读取max_samples处）
     */
    float GetBeforePush(float delay_samples) noexcept {
        return Get(delay_samples);
    }

    /**
     * @param delay_samples 此处不能小于1，否则为非因果滤波器（或者被绕回读取max_samples处）
     */
    template<std::integral T>
    float GetBeforePush(T delay_samples) noexcept {
        int rpos = wpos_ + buffer_.size() - delay_samples;
        int irpos = static_cast<int>(rpos) & mask_;
        return buffer_[irpos];
    }
private:
    float Get(float delay) noexcept {
        if constexpr (INTERPOLATION_TYPE == DelayLineInterp::None) {
            float rpos = wpos_ + buffer_.size() - delay;
            int irpos = static_cast<int>(std::round(rpos)) & mask_;
            return buffer_[irpos];
        }
        else {
            float rpos = wpos_ + buffer_.size() - delay;
            int irpos = static_cast<int>(rpos) & mask_;
            [[maybe_unused]] int inext1 = (irpos + 1) & mask_;
            [[maybe_unused]] int inext2 = (irpos + 2) & mask_;
            [[maybe_unused]] int inext3 = (irpos + 3) & mask_;
            [[maybe_unused]] int iprev1 = (irpos - 1) & mask_;
            [[maybe_unused]] float t = rpos - static_cast<int>(rpos);
            if constexpr (INTERPOLATION_TYPE == DelayLineInterp::Lagrange3rd) {
                return Interpolation::Lagrange3rd(buffer_[irpos], buffer_[inext1], buffer_[inext2], buffer_[inext3], t);
            }
            else if constexpr (INTERPOLATION_TYPE == DelayLineInterp::Linear) {
                return Interpolation::Linear(buffer_[irpos], buffer_[inext1], t);
            }
            else if constexpr (INTERPOLATION_TYPE == DelayLineInterp::PCHIP) {
                return Interpolation::PCHIP(buffer_[iprev1], buffer_[irpos], buffer_[inext1], buffer_[inext2], t);
            }
        }
    }

    std::vector<float> buffer_;
    size_t wpos_{};
    size_t mask_{};
};
}
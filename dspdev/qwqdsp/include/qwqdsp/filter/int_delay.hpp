#pragma once
#include <vector>
#include <cstddef>

namespace qwqdsp::filter {
class IntDelay {
public:
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

    float GetAfterPush(size_t delay_samples) noexcept {
        return Get(delay_samples + 1);
    }

    /**
     * @param delay_samples 此处不能小于1，否则为非因果滤波器（或者被绕回读取max_samples处）
     */
    float GetBeforePush(size_t delay_samples) noexcept {
        int rpos = wpos_ + buffer_.size() - delay_samples;
        int irpos = static_cast<int>(rpos) & mask_;
        return buffer_[irpos];
    }
private:
    float Get(size_t delay) noexcept {
        size_t rpos = wpos_ + buffer_.size() - delay;
        size_t irpos = rpos & mask_;
        return buffer_[irpos];
    }

    std::vector<float> buffer_;
    size_t wpos_{};
    size_t mask_{};
};
}
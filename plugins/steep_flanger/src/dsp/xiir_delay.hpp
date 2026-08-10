#pragma once
#include <cmath>
#include <vector>

namespace steep_flanger {
class XIirDelayLine {
public:
    void Init(float max_ms, float fs) {
        float d = max_ms * fs / 1000.0f;
        size_t i = static_cast<size_t>(std::ceil(d));
        Init(i);
    }

    void Init(size_t max_samples) {
        size_t a = 1;
        while (a < max_samples) {
            a *= 2;
        }
        mask_ = static_cast<int>(a - 1);
        delay_length_ = static_cast<int>(a);
        buffer_.resize(a * 4);
    }

    void Reset() noexcept {
        wpos_ = 0;
        std::fill(buffer_.begin(), buffer_.end(), float{});
    }

    template <int kChannel>
    float GetBeforePush(float delay_samples) const noexcept {
        if constexpr (kChannel == 0) {
            float frpos = static_cast<float>(wpos_ + delay_length_) - delay_samples;
            auto t = frpos - std::floor(frpos);
            auto rpos = static_cast<int>(frpos);
            auto irpos = rpos & mask_;
            auto rprev1 = (irpos - 1) & (mask_);
            auto rnext1 = (irpos + 1) & (mask_);
            auto rnext2 = (irpos + 2) & (mask_);

            auto yn1 = buffer_[static_cast<size_t>(rprev1)];
            auto y0 = buffer_[static_cast<size_t>(irpos)];
            auto y1 = buffer_[static_cast<size_t>(rnext1)];
            auto y2 = buffer_[static_cast<size_t>(rnext2)];

            auto d0 = (y1 - yn1) * 0.5f;
            auto d1 = (y2 - y0) * 0.5f;
            auto d = y1 - y0;
            auto m0 = 3.0f * d - 2.0f * d0 - d1;
            auto m1 = d0 - 2.0f * d + d1;
            return y0 + t * (d0 + t * (m0 + t * m1));
        }
        else {
            float frpos = static_cast<float>(wpos_ + delay_length_) - delay_samples;
            auto t = frpos - std::floor(frpos);
            auto rpos = static_cast<int>(frpos);
            auto irpos = rpos & mask_;
            auto rprev1 = (irpos - 1) & (mask_);
            auto rnext1 = (irpos + 1) & (mask_);
            auto rnext2 = (irpos + 2) & (mask_);

            auto* ptr = buffer_.data() + delay_length_ * 2;
            auto yn1 = ptr[static_cast<size_t>(rprev1)];
            auto y0 = ptr[static_cast<size_t>(irpos)];
            auto y1 = ptr[static_cast<size_t>(rnext1)];
            auto y2 = ptr[static_cast<size_t>(rnext2)];

            auto d0 = (y1 - yn1) * 0.5f;
            auto d1 = (y2 - y0) * 0.5f;
            auto d = y1 - y0;
            auto m0 = 3.0f * d - 2.0f * d0 - d1;
            auto m1 = d0 - 2.0f * d + d1;
            return y0 + t * (d0 + t * (m0 + t * m1));
        }
    }

    void Push(float xleft, float xright) noexcept {
        wpos_ = (wpos_ + 1) & mask_;
        buffer_[static_cast<size_t>(wpos_)] = xleft;
        buffer_[static_cast<size_t>(wpos_ + delay_length_)] = xleft;
        int wpos2 = wpos_ + delay_length_ + delay_length_;
        buffer_[static_cast<size_t>(wpos2)] = xright;
        buffer_[static_cast<size_t>(wpos2 + delay_length_)] = xright;
    }
private:
    std::vector<float> buffer_;
    int delay_length_{};
    int wpos_{};
    int mask_{};
};
} // namespace steep_flanger

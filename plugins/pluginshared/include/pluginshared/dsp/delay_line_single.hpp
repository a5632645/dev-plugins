#pragma once
#include <vector>
#include "../simd.hpp"
#include "../align_allocator.hpp"

namespace pluginshared::dsp {
template <simd::IsSimdFloat SimdT>
class DelayLineSingle {
public:
    void Init(float max_ms, float fs) {
        float d = max_ms * fs / 1000.0f;
        size_t i = static_cast<size_t>(d);
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
        mask_ = static_cast<int>(a - 1);
    }

    void Reset() noexcept {
        wpos_ = 0;
        std::fill(buffer_.begin(), buffer_.end(), SimdT{});
    }

    void Push(SimdT x) noexcept {
        buffer_[static_cast<size_t>(wpos_)] = x;
        wpos_ = (wpos_ + 1) & mask_;
    }

    SimdT GetAfterPush(float delay_samples) noexcept {
        // return Get(delay_samples + 1);
        float rpos = static_cast<float>(wpos_ + mask_) - delay_samples;
        int irpos = static_cast<int>(rpos) & mask_;
        int iprev1 = (irpos - 1) & (mask_);
        int inext1 = (irpos + 1) & (mask_);
        int inext2 = (irpos + 2) & (mask_);
        float t = rpos - std::floor(rpos);

        auto yn1 = buffer_[static_cast<size_t>(iprev1)];
        auto y0 = buffer_[static_cast<size_t>(irpos)];
        auto y1 = buffer_[static_cast<size_t>(inext1)];
        auto y2 = buffer_[static_cast<size_t>(inext2)];

        auto d0 = (y1 - yn1) * 0.5f;
        auto d1 = (y2 - y0) * 0.5f;
        auto d = y1 - y0;
        auto m0 = 3.0f * d - 2.0f * d0 - d1;
        auto m1 = d0 - 2.0f * d + d1;
        return y0 + t * (d0 + t * (m0 + t * m1));
    }

    /**
     * @param delay_samples 此处不能小于1，否则为非因果滤波器（或者被绕回读取max_samples处）
     */
    SimdT GetBeforePush(float delay_samples) noexcept {
        // return Get(delay_samples);
        float rpos = static_cast<float>(wpos_ + mask_ + 1) - delay_samples;
        int irpos = static_cast<int>(rpos) & mask_;
        int iprev1 = (irpos - 1) & mask_;
        int inext1 = (irpos + 1) & mask_;
        int inext2 = (irpos + 2) & mask_;
        float t = rpos - static_cast<float>(irpos);

        auto yn1 = buffer_[static_cast<size_t>(iprev1)];
        auto y0 = buffer_[static_cast<size_t>(irpos)];
        auto y1 = buffer_[static_cast<size_t>(inext1)];
        auto y2 = buffer_[static_cast<size_t>(inext2)];

        auto d0 = (y1 - yn1) * 0.5f;
        auto d1 = (y2 - y0) * 0.5f;
        auto d = y1 - y0;
        auto m0 = 3.0f * d - 2.0f * d0 - d1;
        auto m1 = d0 - 2.0f * d + d1;
        return y0 + t * (d0 + t * (m0 + t * m1));
    }
private:
    std::vector<SimdT, simd::AlignedAllocator<SimdT, alignof(SimdT)>> buffer_;
    int wpos_{};
    int mask_{};
};

} // namespace pluginshared::dsp

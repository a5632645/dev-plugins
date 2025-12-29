#pragma once
#include <vector>
#include "simd_pack.hpp"

namespace qwqdsp_simd_element {
/**
 * @brief 多通道存储，单变量延迟
 * 
 * @tparam N 
 */
template<size_t N>
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
        mask_ = static_cast<uint32_t>(a - 1);
    }

    void Reset() noexcept {
        wpos_ = 0;
        std::fill(buffer_.begin(), buffer_.end(), PackFloat<N>{});
    }

    void Push(PackFloatCRef<N> x) noexcept {
        buffer_[wpos_] = x;
        wpos_ = (wpos_ + 1) & mask_;
    }

    PackFloat<N> GetAfterPush(float delay_samples) noexcept {
        // return Get(delay_samples + 1);
        float rpos = static_cast<float>(wpos_ + mask_) - delay_samples;
        uint32_t irpos = static_cast<uint32_t>(rpos) & mask_;
        uint32_t iprev1 = (irpos - 1u) & (mask_);
        uint32_t inext1 = (irpos + 1u) & (mask_);
        uint32_t inext2 = (irpos + 2u) & (mask_);
        float t = rpos - std::floor(rpos);
        
        auto yn1 = buffer_[iprev1];
        auto y0 = buffer_[irpos];
        auto y1 = buffer_[inext1];
        auto y2 = buffer_[inext2];

        auto d0 = (y1 - yn1) * 0.5f;
        auto d1 = (y2 - y0) * 0.5f;
        auto d = y1 - y0;
        auto m0 = 3.0f * d - 2.0f * d0 - d1;
        auto m1 = d0 - 2.0f * d + d1;
        return y0 + t * (
            d0 + t * (
                m0 + t * m1
            )
        );
    }

    /**
     * @param delay_samples 此处不能小于1，否则为非因果滤波器（或者被绕回读取max_samples处）
     */
    PackFloat<N> GetBeforePush(float delay_samples) noexcept {
        // return Get(delay_samples);
        float rpos = static_cast<float>(wpos_ + buffer_.size()) - delay_samples;
        uint32_t irpos = static_cast<uint32_t>(rpos) & mask_;
        uint32_t iprev1 = (irpos - 1) & mask_;
        uint32_t inext1 = (irpos + 1) & mask_;
        uint32_t inext2 = (irpos + 2) & mask_;
        float t = rpos - static_cast<float>(irpos);

        auto yn1 = buffer_[iprev1];
        auto y0 = buffer_[irpos];
        auto y1 = buffer_[inext1];
        auto y2 = buffer_[inext2];

        auto d0 = (y1 - yn1) * 0.5f;
        auto d1 = (y2 - y0) * 0.5f;
        auto d = y1 - y0;
        auto m0 = 3.0f * d - 2.0f * d0 - d1;
        auto m1 = d0 - 2.0f * d + d1;
        return y0 + t * (
            d0 + t * (
                m0 + t * m1
            )
        );
    }
private:
    std::vector<PackFloat<N>> buffer_;
    uint32_t wpos_{};
    uint32_t mask_{};
};

} // namespace qwqdsp_simd_element

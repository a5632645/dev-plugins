#pragma once
#include <vector>
#include <array>

namespace qwqdsp::adaptive {
class LagBuffer {
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
        wpos_ = mask_;
        std::fill(buffer_.begin(), buffer_.end(), 0.0f);
    }

    void Push(float x) noexcept {
        buffer_[wpos_--] = x;
        wpos_ &= mask_;
    }

    float operator[](size_t lag) const noexcept {
        return buffer_[(wpos_ + lag + 1) & mask_];
    }
private:
    std::vector<float> buffer_;
    size_t wpos_{};
    size_t mask_{};
};

template<size_t kSize>
class LagBufferStatic {
public:
    static constexpr size_t kMask = kSize - 1;

    void Reset() noexcept {
        wpos_ = kMask;
        std::fill(buffer_.begin(), buffer_.end(), 0.0f);
    }

    void Push(float x) noexcept {
        buffer_[wpos_--] = x;
        wpos_ &= kMask;
    }

    /**
     * @note 在Push后再使用
     */
    float operator[](size_t lag) const noexcept {
        return buffer_[(wpos_ + lag + 1) & kMask];
    }
private:
    std::array<float, kSize> buffer_{};
    size_t wpos_{};
};
}
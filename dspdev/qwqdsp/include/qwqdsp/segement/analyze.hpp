#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <span>

namespace qwqdsp::segement {
/**
 * @brief 仅支持分析的分块处理
 */
template<size_t kMaxBufferSize>
class Analyze {
public:
    void SetSize(size_t new_size) noexcept {
        size_ = new_size;
    }

    size_t GetSize() const noexcept {
        return size_;
    }

    void SetHop(size_t hop) noexcept {
        hop_size_ = hop;
    }

    size_t GetHop() const noexcept {
        return hop_size_;
    }

    void Push(std::span<const float> block) noexcept {
        std::copy(block.begin(), block.end(), buffer_.begin() + num_input_);
        num_input_ += block.size();
    }

    bool CanProcess() const noexcept {
        return num_input_ >= size_;
    }

    std::span<float> GetBlock() noexcept {
        return {buffer_.data(), size_};
    }

    void Advance() noexcept {
        num_input_ -= hop_size_;
        for (size_t i = 0; i < num_input_; i++) {
            buffer_[i] = buffer_[i + hop_size_];
        }
    }

    void Reset() noexcept {
        num_input_ = 0;
    }
private:
    size_t size_{};
    size_t hop_size_{};
    size_t num_input_{};
    std::array<float, kMaxBufferSize> buffer_{};
};
}
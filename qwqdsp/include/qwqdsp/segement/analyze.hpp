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
    void SetSize(size_t new_size) {
        size_ = new_size;
    }

    void SetHop(size_t hop) {
        hop_size_ = hop;
    }

    void Push(std::span<const float> block) {
        std::copy(block.begin(), block.end(), buffer_.begin() + num_input_);
        num_input_ += block.size();
    }

    bool CanProcess() const {
        return num_input_ >= size_;
    }

    std::span<float> GetBlock() {
        return {buffer_.data(), size_};
    }

    void Advance() {
        num_input_ -= hop_size_;
        for (size_t i = 0; i < num_input_; i++) {
            buffer_[i] = buffer_[i + hop_size_];
        }
    }

    void Reset() {
        num_input_ = 0;
    }
private:
    size_t size_{};
    size_t hop_size_{};
    size_t num_input_{};
    std::array<float, kMaxBufferSize> buffer_{};
};
}
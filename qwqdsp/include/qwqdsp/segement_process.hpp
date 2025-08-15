#pragma once
#include <array>
#include <cstddef>
#include <span>

namespace qwqdsp {
class SegementProcess {
public:
    void SetSize(size_t new_size) {
        size_ = new_size;
    }

    void SetHop(size_t hop) {
        hop_size_ = hop;
    }

    void Push(std::span<const float> block) {
        std::copy(block.begin(), block.end(), buffer_.begin() + numInput_);
        numInput_ += block.size();
    }

    bool CanProcess() const {
        return numInput_ >= size_;
    }

    std::span<float> GetBlock() {
        return {buffer_.data(), size_};
    }

    void Advance() {
        numInput_ -= hop_size_;
        for (size_t i = 0; i < numInput_; i++) {
            buffer_[i] = buffer_[i + hop_size_];
        }
    }
private:
    size_t size_{};
    size_t hop_size_{};
    size_t numInput_{};
    std::array<float, 32768> buffer_{};
};
}
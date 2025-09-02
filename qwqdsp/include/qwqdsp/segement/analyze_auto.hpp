#pragma once
#include <cstddef>
#include <span>
#include <cmath>
#include "slice.hpp"

namespace qwqdsp::segement {
/**
 * @brief 仅支持分析的自动分块
 * @tparam kOffline true: 会将不足的部分也处理. false:适合实时音频流
 */
template<bool kOffline>
class AnalyzeAuto {
public:
    /**
     * @tparam Func void(std::span<const float> block)
     */
    template<class Func>
    void Process(
        std::span<const float> block,
        Func&& func
    ) noexcept(noexcept(func(std::declval<std::span<const float>>()))) {
        Slice1D input{block};
        while (!input.IsEnd()) {
            size_t need = size_ - input_wpos_;
            auto in = input.GetSome(need);
            std::copy(in.begin(), in.end(), input_buffer_.begin() + input_wpos_);
            input_wpos_ += in.size();
            if (input_wpos_ >= size_) {
                func({input_buffer_.data(), size_});
                input_wpos_ -= hop_;
                for (int i = 0; i < input_wpos_; i++) {
                    input_buffer_[i] = input_buffer_[i + hop_];
                }
            }
        }
        if constexpr (kOffline) {
            while (input_wpos_ > 0) {
                size_t need = size_ - input_wpos_;
                std::fill_n(input_buffer_.begin() + input_wpos_, need, 0.0f);
                input_wpos_ -= std::min(input_wpos_, hop_);
                for (int i = 0; i < input_wpos_; i++) {
                    input_buffer_[i] = input_buffer_[i + hop_];
                }
                func({input_buffer_.data(), size_});
            }
        }
    }

    size_t GetMinFrameSize(size_t input_size) const noexcept {
        return std::ceil(static_cast<float>(input_size) / hop_);
    }

    void SetSize(size_t size) noexcept {
        size_ = size;
        if (input_buffer_.size() < size) {
            input_buffer_.resize(size);
        }
    }

    void SetHop(size_t hop) noexcept {
        hop_ = hop;
    }

    void Reset() noexcept {
        input_wpos_ = 0;
    }
private:
    std::vector<float> input_buffer_;
    size_t size_{};
    size_t hop_{};
    size_t input_wpos_{};
};
}
#pragma once
#include <cstddef>
#include <span>
#include <cmath>
#include "audio.hpp"

namespace qwqdsp::segement {
/**
 * @brief 仅支持分析的自动分块
 * @tparam kOffline true: 会将不足的部分也处理，只能用一次. false:适合实时音频流
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
    ) {
        SliceMono<const float> input{block};
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

    size_t GetMinFrameSize(size_t input_size) {
        return std::ceil(static_cast<float>(input_size) / hop_);
    }

    void SetSize(size_t size) {
        size_ = size;
        input_buffer_.resize(size_);
    }

    void SetHop(size_t hop) {
        hop_ = hop;
    }
private:
    std::vector<float> input_buffer_;
    size_t size_{};
    size_t hop_{};
    size_t input_wpos_{};
};
}
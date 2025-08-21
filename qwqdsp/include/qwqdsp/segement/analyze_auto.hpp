#pragma once
#include <cstddef>
#include <span>
#include "audio.hpp"

namespace qwqdsp::segement {
/**
 * @brief 仅支持分析的自动分块
 */
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
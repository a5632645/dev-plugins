#pragma once
#include <algorithm>
#include <cstddef>
#include <span>
#include "slice.hpp"

namespace qwqdsp::segement {
/**
 * @brief 适用于实时处理的分析合成
 */
class AnalyzeSynthsisOnline {
public:
    /**
     * @tparam Func void(std::span<const float> input, std::span<float> output)
     */
    template<class Func>
    void Process(
        std::span<float> block,
        Func&& func
    ) noexcept(noexcept(func(std::declval<std::span<const float>>(), std::declval<std::span<float>>()))) {
        Slice1D input{block};
        while (!input.IsEnd()) {
            size_t need = size_ - input_wpos_;
            auto in = input.GetSome(need);
            std::copy(in.begin(), in.end(), input_buffer_.begin() + input_wpos_);
            input_wpos_ += in.size();
            if (input_wpos_ >= size_) {
                std::copy(input_buffer_.begin(), input_buffer_.end(), process_buffer_.begin());
                func(std::span<const float>{input_buffer_.data(), size_}, std::span<float>{process_buffer_.data(), size_});
                input_wpos_ -= hop_;
                for (int i = 0; i < input_wpos_; i++) {
                    input_buffer_[i] = input_buffer_[i + hop_];
                }
                for (int i = 0; i < size_; i++) {
                    output_buffer_[i + write_add_end_] += process_buffer_[i];
                }
                write_end_ = write_add_end_ + size_;
                write_add_end_ += hop_;
            }

            if (write_add_end_ >= in.size()) {
                // extract output
                int extractSize = in.size();
                for (int i = 0; i < extractSize; ++i) {
                    in[i] = output_buffer_[i] / ((float)size_ / hop_);
                }
                
                // shift output buffer
                int shiftSize = write_end_ - extractSize;
                for (int i = 0; i < shiftSize; i++) {
                    output_buffer_[i] = output_buffer_[i + extractSize];
                }
                write_add_end_ -= extractSize;
                int newWriteEnd = write_end_ - extractSize;
                // zero shifed buffer
                for (int i = newWriteEnd; i < write_end_; ++i) {
                    output_buffer_[i] = 0.0f;
                }
                write_end_ = newWriteEnd;
            }
            else {
                // zero buffer
                std::fill(in.begin(), in.end(), 0.0f);
            }
        }
    }

    void SetSize(size_t size) noexcept {
        size_ = size;
        if (input_buffer_.size() < size) {
            input_buffer_.resize(size);
        }
        if (output_buffer_.size() < (size + hop_) * 2) {
            output_buffer_.resize((size + hop_) * 2);
        }
        process_buffer_.resize(size_);
    }

    void SetHop(size_t hop) noexcept {
        hop_ = hop;
    }

    void Reset() noexcept {
        std::fill_n(output_buffer_.begin(), write_end_, 0.0f);
        input_wpos_ = 0;
        write_end_ = 0;
        write_add_end_ = 0;
    }
private:
    std::vector<float> input_buffer_;
    std::vector<float> process_buffer_;
    std::vector<float> output_buffer_;
    size_t size_{};
    size_t hop_{};
    size_t input_wpos_{};
    size_t write_end_{};
    size_t write_add_end_{};
};
}
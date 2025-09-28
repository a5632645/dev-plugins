#pragma once
#include <cstddef>
#include <span>
#include <vector>
#include <cmath>
#include "slice.hpp"

namespace qwqdsp::segement {
/**
 * @brief 适用于离线处理的分析合成
 */
class AnalyzeSynthsisOffline {
public:
    /**
     * @tparam Func void(std::span<const float> input, std::span<float> output)
     */
    template<class Func>
    void Process(
        std::span<const float> in_span,
        std::vector<float>& output,
        Func&& func
    ) noexcept(noexcept(func(std::declval<std::span<const float>>(), std::declval<std::span<float>>()))) {
        // 处理音频
        {
            Slice1D input{in_span};
            while (!input.IsEnd()) {
                size_t need = size_ - input_wpos_;
                auto in = input.GetSome(need);
                std::copy(in.begin(), in.end(), input_buffer_.begin() + input_wpos_);
                input_wpos_ += in.size();
                if (input_wpos_ >= size_) {
                    std::copy(input_buffer_.begin(), input_buffer_.end(), process_buffer_.begin());
                    func(std::span<const float>{input_buffer_.data(), size_}, std::span<float>{process_buffer_.data(), size_});
                    input_wpos_ -= input_hop_;
                    for (int i = 0; i < input_wpos_; i++) {
                        input_buffer_[i] = input_buffer_[i + input_hop_];
                    }
                    for (int i = 0; i < size_; i++) {
                        output_buffer_[i + write_add_end_] += process_buffer_[i];
                    }
                    write_end_ = write_add_end_ + size_;
                    write_add_end_ += output_hop_;
                }
    
                if (write_add_end_ >= 0) {
                    // extract output
                    int extractSize = write_add_end_;
                    for (int i = 0; i < extractSize; ++i) {
                        output.emplace_back(output_buffer_[i] / ((float)size_ / output_hop_));
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
            }
        }
        // 填充一堆0
        {
            while (input_wpos_ > 0) {
                size_t need = size_ - input_wpos_;
                std::fill_n(input_buffer_.begin() + input_wpos_, need, 0.0f);
                std::copy(input_buffer_.begin(), input_buffer_.end(), process_buffer_.begin());
                input_wpos_ -= std::min(input_wpos_, input_hop_);
                for (int i = 0; i < input_wpos_; i++) {
                    input_buffer_[i] = input_buffer_[i + input_hop_];
                }
                func(std::span<const float>{input_buffer_.data(), size_}, std::span<float>{process_buffer_.data(), size_});
                for (int i = 0; i < size_; i++) {
                    output_buffer_[i + write_add_end_] += process_buffer_[i];
                }
                write_end_ = write_add_end_ + size_;
                write_add_end_ += output_hop_;
    
                if (write_add_end_ >= 0) {
                    // extract output
                    int extractSize = write_add_end_;
                    for (int i = 0; i < extractSize; ++i) {
                        output.emplace_back(output_buffer_[i] / ((float)size_ / output_hop_));
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
            }
        }
        // 拿走圣遗物
        {
            for (size_t i = 0; i < write_end_; i++) {
                output.emplace_back(output_buffer_[i] / ((float)size_ / output_hop_));
            }
        }
    }

    size_t GetMinOutputSize(size_t input_size) const noexcept {
        size_t num_frame = std::ceil(static_cast<float>(input_size) / input_hop_);
        return (num_frame - 1) * output_hop_ + size_;
    }

    void SetSize(size_t size) noexcept {
        size_ = size;
        if (input_buffer_.size() < size) {
            input_buffer_.resize(size);
        }
        if (output_buffer_.size() < (size + output_hop_) * 2) {
            output_buffer_.resize((size + output_hop_) * 2);
        }
        process_buffer_.resize(size_);
    }

    void SetInputHop(size_t hop) noexcept {
        input_hop_ = hop;
    }

    void SetOutputHop(size_t hop) noexcept {
        output_hop_ = hop;
    }

    void Reset() noexcept {
        std::fill_n(output_buffer_.begin(), write_end_, 0.0f);
        input_wpos_ = 0;
        write_add_end_ = 0;
        write_end_ = 0;
    }
private:
    std::vector<float> input_buffer_;
    std::vector<float> process_buffer_;
    std::vector<float> output_buffer_;
    size_t size_{};
    size_t input_hop_{};
    size_t output_hop_{};
    size_t input_wpos_{};
    size_t write_end_{};
    size_t write_add_end_{};
};
}
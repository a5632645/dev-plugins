#pragma once
#include <algorithm>
#include <cstddef>
#include <ranges>
#include <span>
#include <vector>

namespace green_vocoder::dsp {

// ------------------------------------------------------------
// BlockOLA：块驱动 OLA（Overlap-Add）框架
// ------------------------------------------------------------
// 负责块驱动处理的输入分帧（hop）、合成加窗、重叠相加输出缓冲与
// 输出抽取。块处理算法（BlockBurgLPC、STFT 等）通过 Process 传入
// 的 BlockFunc 处理每一帧，BlockFunc 返回的帧由本类加窗并叠加进
// 输出缓冲，最后按输入长度抽取输出。
template <typename Sample>
class BlockOLA {
public:
    // block_size：帧长；hop_size：帧间步进；synthesis_window：合成窗（长度须为 block_size）
    void Init(size_t block_size, size_t hop_size, std::span<const float> synthesis_window) {
        block_size_ = block_size;
        hop_size_ = hop_size;
        main_input_buffer_.resize(block_size_);
        side_input_buffer_.resize(block_size_);
        output_buffer_.resize(block_size_ * 4);
        window_.assign(synthesis_window.begin(), synthesis_window.end());
        Reset();
    }

    void Reset() noexcept {
        std::ranges::fill(main_input_buffer_, Sample{});
        std::ranges::fill(side_input_buffer_, Sample{});
        std::ranges::fill(output_buffer_, Sample{});
        num_input_ = 0;
        write_end_ = 0;
        write_add_begin_ = 0;
    }

    // 输出重建增益（WOLA 归一化常数，如 4 倍重叠 hann 窗的 0.25 / 4.0）
    void SetOutputGain(float gain) noexcept { output_gain_ = gain; }
    float GetOutputGain() const noexcept { return output_gain_; }

    size_t GetBlockSize() const noexcept { return block_size_; }
    size_t GetHopSize() const noexcept { return hop_size_; }

    // 处理一段输入；攒够一帧时调用 BlockFunc(main_frame, side_frame)，
    // 返回待重叠相加的处理后帧（长度 block_size_）。
    template <typename BlockFunc>
    void Process(Sample* main, Sample* side, size_t num_samples, BlockFunc&& block_func) {
        // 输入分帧缓冲
        std::copy_n(main, num_samples, main_input_buffer_.begin() + static_cast<std::ptrdiff_t>(num_input_));
        std::copy_n(side, num_samples, side_input_buffer_.begin() + static_cast<std::ptrdiff_t>(num_input_));
        num_input_ += num_samples;
        while (num_input_ >= block_size_) {
            std::span<Sample const> main_frame{main_input_buffer_.data(), block_size_};
            std::span<Sample const> side_frame{side_input_buffer_.data(), block_size_};
            std::span<Sample const> out_frame = block_func(main_frame, side_frame);

            // 输入缓冲前进一帧
            num_input_ -= hop_size_;
            for (size_t i = 0; i < num_input_; ++i)
                main_input_buffer_[i] = main_input_buffer_[i + hop_size_];
            for (size_t i = 0; i < num_input_; ++i)
                side_input_buffer_[i] = side_input_buffer_[i + hop_size_];

            // 合成加窗 + 重叠相加
            for (size_t i = 0; i < block_size_; ++i)
                output_buffer_[i + write_add_begin_] += out_frame[i] * window_[i];
            write_end_ = write_add_begin_ + block_size_;
            write_add_begin_ += hop_size_;
        }

        // 输出抽取
        if (write_add_begin_ >= num_samples) {
            for (size_t i = 0; i < num_samples; ++i)
                main[i] = output_buffer_[i] * output_gain_;
            size_t const shift_size = write_end_ - num_samples;
            for (size_t i = 0; i < shift_size; ++i)
                output_buffer_[i] = output_buffer_[i + num_samples];
            write_add_begin_ -= num_samples;
            size_t const new_write_end = write_end_ - num_samples;
            for (size_t i = new_write_end; i < write_end_; ++i)
                output_buffer_[i] = Sample{};
            write_end_ = new_write_end;
        }
        else {
            std::fill_n(main, num_samples, Sample{});
        }
    }

private:
    std::vector<Sample> main_input_buffer_{};
    std::vector<Sample> side_input_buffer_{};
    std::vector<Sample> output_buffer_{};
    std::vector<float> window_{};
    size_t block_size_{};
    size_t hop_size_{};
    size_t num_input_{};
    size_t write_end_{};
    size_t write_add_begin_{};
    float output_gain_{1.0f};
};

} // namespace green_vocoder::dsp

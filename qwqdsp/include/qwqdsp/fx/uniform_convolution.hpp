#pragma once
#include <algorithm>
#include <cstddef>
#include "qwqdsp/spectral/real_fft.hpp"
#include "qwqdsp/segement/analyze_auto.hpp"
#include "qwqdsp/segement/slice.hpp"
#include "qwqdsp/window/helper.hpp"

namespace qwqdsp::fx {
class UniformConvolution {
public:
    void Init(size_t latency) {
        size_t fft_size = latency * 2;
        block_size_ = latency;
        fft_.Init(fft_size);
        if (input_buffer_.size() < latency) {
            input_buffer_.resize(latency);
        }
        if (output_buffer_.size() < fft_size * 2) {
            output_buffer_.resize(fft_size * 2);
        }
        process_buffer_.resize(fft_size);
        Reset();
    }

    void Reset() noexcept {
        std::fill_n(output_buffer_.begin(), write_end_, 0.0f);
        for (auto& f : input_frames_) {
            std::fill(f.begin(), f.end(), std::complex<float>{});
        }
        input_wpos_ = 0;
        input_frame_wpos_ = 0;
        write_end_ = 0;
        write_add_end_ = 0;
    }

    void SetIR(std::span<float> ir) noexcept {
        segement::AnalyzeAuto<true> analyze;
        analyze.SetSize(block_size_);
        analyze.SetHop(block_size_);
        size_t num_frame = analyze.GetMinFrameSize(ir.size());
        ir_frames_.resize(num_frame);
        input_frames_.resize(num_frame);
        for (size_t i = 0; i < num_frame; ++i) {
            ir_frames_[i].resize(fft_.NumBins());
            input_frames_[i].resize(fft_.NumBins());
        }
        output_frame_.resize(fft_.NumBins());
        size_t i = 0;
        analyze.Process(ir, [this, &i](std::span<const float> block) {
            window::Helper::ZeroPad(process_buffer_, block);
            fft_.FFT(process_buffer_, ir_frames_[i]);
            ++i;
        });
        Reset();
    }

    void Process(std::span<float> block) noexcept {
        segement::Slice1D input{block};
        while (!input.IsEnd()) {
            size_t need = block_size_ - input_wpos_;
            auto in = input.GetSome(need);
            std::copy(in.begin(), in.end(), input_buffer_.begin() + input_wpos_);
            input_wpos_ += in.size();
            if (input_wpos_ >= block_size_) {
                window::Helper::ZeroPad(process_buffer_, input_buffer_);
                fft_.FFT(process_buffer_, input_frames_[input_frame_wpos_]);
                input_wpos_ -= block_size_;

                const size_t num_bins = fft_.NumBins();
                for (size_t i = 0; i < num_bins; ++i) {
                    output_frame_[i] = input_frames_[input_frame_wpos_][i] * ir_frames_[0][i];
                }
                for (size_t i = 1; i < ir_frames_.size(); ++i) {
                    size_t idx = input_frame_wpos_ + input_frames_.size() - i;
                    if (idx >= input_frames_.size()) {
                        idx -= input_frames_.size();
                    }
                    for (size_t j = 0; j < num_bins; ++j) {
                        output_frame_[j] += input_frames_[idx][j] * ir_frames_[i][j];
                    }
                }

                fft_.IFFT(process_buffer_, output_frame_);
                for (int i = 0; i < block_size_ * 2; i++) {
                    output_buffer_[i + write_add_end_] += process_buffer_[i];
                }
                write_end_ = write_add_end_ + block_size_ * 2;
                write_add_end_ += block_size_;
                ++input_frame_wpos_;
                if (input_frame_wpos_ >= ir_frames_.size()) {
                    input_frame_wpos_ = 0;
                }
            }

            if (write_add_end_ >= in.size()) {
                // extract output
                int extractSize = in.size();
                for (int i = 0; i < extractSize; ++i) {
                    in[i] = output_buffer_[i];
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
private:
    using Frame = std::vector<std::complex<float>>;

    size_t block_size_{};
    size_t input_wpos_{};
    size_t write_end_{};
    size_t write_add_end_{};
    std::vector<float> input_buffer_;
    std::vector<float> process_buffer_;
    std::vector<float> output_buffer_;

    spectral::RealFFT fft_;
    std::vector<Frame> ir_frames_;
    std::vector<Frame> input_frames_;
    size_t input_frame_wpos_{};
    Frame output_frame_;
};
}
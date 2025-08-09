#pragma once
#include "delay_line.hpp"
#include "noise.hpp"
#include "smoother.hpp"
#include <algorithm>
#include <cstddef>
#include <array>
#include <span>

namespace dsp {
class LatticeBlock {
public:
    void Init(float max_ms, float fs) {
        delay_.Init(max_ms, fs);
    }

    float TickUp(float x) {
        e_ = delay_.GetBeforePush(delay_samples_);
        up_out_ = x + k_ * e_;
        return up_out_;
    }

    float TickDown(float x) {
        delay_.Push(x);
        return e_ - k_ * up_out_;
    }

    void SetK(float k) {
        k_ = k;
    }

    void SetDelaySamples(float d) {
        delay_samples_ = d;
    }

    void SetDelay(float ms, float fs) {
        SetDelaySamples(ms * fs / 1000.0f);
    }
private:
    float k_{};
    float delay_samples_{};
    float up_out_{};
    float e_{};
    dsp::DelayLine<true> delay_;
};

class LatticeAPF {
public:
    static constexpr size_t kNumBlock = 16;

    void Init(float max_ms, float fs) {
        for (auto& b : left_block_) {
            b.Init(max_ms, fs);
        }
        for (auto& n : left_noise_) {
            n.Init(fs);
            n.Reset();
        }
        for (auto& b : rigt_block_) {
            b.Init(max_ms, fs);
        }
        for (auto& n : right_noise_) {
            n.Init(fs);
            n.Reset();
        }
        fs_ = fs;
        begin_smooth_.SetSmoothTime(50.0f, fs);
        end_smooth_.SetSmoothTime(50.0f, fs);
        for (size_t i = 0; i < kNumBlock; ++i) {
            k_smooth_[i].SetSmoothTime(10.0f + 10.0f * i, fs);
        }
    }

    void Process(std::span<float> left, std::span<float> right) {
        size_t num_samples = left.size();
        if (mono_modulator_) {
            for (size_t i = 0; i < num_samples; ++i) {
                float begin_time = begin_smooth_.Tick();
                float end_time = end_smooth_.Tick();
                for (size_t i = 0; i < num_block_; ++i) {
                    float k = k_smooth_[i].Tick();
                    left_block_[i].SetK(k);
                    rigt_block_[i].SetK(k);
                }

                float x = left[i];
                float xr = right[i];
                float y = x;
                float yr = xr;
                // update delay time
                for (size_t i = 0; i < num_block_; ++i) {
                    left_block_[i].SetDelay(std::lerp(begin_time, end_time, left_noise_[i].Tick()), fs_);
                    rigt_block_[i].SetDelay(std::lerp(begin_time, end_time, left_noise_[i].Tick()), fs_);
                }

                // process
                for (size_t i = 0; i < num_block_; ++i) {
                    x = left_block_[i].TickUp(x);
                    xr = rigt_block_[i].TickUp(xr);
                }
                for (int i = num_block_ - 1; i >= 0; --i) {
                    x = left_block_[i].TickDown(x);
                    xr = rigt_block_[i].TickDown(xr);
                }
                x = std::clamp(x, -4.0f, 4.0f);
                xr = std::clamp(xr, -4.0f, 4.0f);
                left[i] = std::lerp(y, x, mix_);
                right[i] = std::lerp(yr, xr, mix_);
            }
        }
        else {
            for (size_t i = 0; i < num_samples; ++i) {
                float begin_time = begin_smooth_.Tick();
                float end_time = end_smooth_.Tick();
                for (size_t i = 0; i < num_block_; ++i) {
                    float k = k_smooth_[i].Tick();
                    left_block_[i].SetK(k);
                    rigt_block_[i].SetK(k);
                }

                float x = left[i];
                float xr = right[i];
                float y = x;
                float yr = xr;
                // update delay time
                for (size_t i = 0; i < num_block_; ++i) {
                    left_block_[i].SetDelay(std::lerp(begin_time, end_time, left_noise_[i].Tick()), fs_);
                    rigt_block_[i].SetDelay(std::lerp(begin_time, end_time, right_noise_[i].Tick()), fs_);
                }

                // process
                for (size_t i = 0; i < num_block_; ++i) {
                    x = left_block_[i].TickUp(x);
                    xr = rigt_block_[i].TickUp(xr);
                }
                for (int i = num_block_ - 1; i >= 0; --i) {
                    x = left_block_[i].TickDown(x);
                    xr = rigt_block_[i].TickDown(xr);
                }
                x = std::clamp(x, -4.0f, 4.0f);
                xr = std::clamp(xr, -4.0f, 4.0f);
                left[i] = std::lerp(y, x, mix_);
                right[i] = std::lerp(yr, xr, mix_);
            }
        }
    }

    // --------------------------------------------------------------------------------
    // set
    // --------------------------------------------------------------------------------
    void SetNumBlock(size_t num) {
        num_block_ = num;
        _CalcK();
    }

    void SetReflection(float r) {
        used_k_ = r;
        _CalcK();
    }

    void SetDelayBegin(float begin) {
        begin_smooth_.SetTarget(begin);
    }

    void SetDelayEnd(float end) {
        end_smooth_.SetTarget(end);
    }

    void SetFrequency(float freq) {
        for (auto& n : left_noise_) {
            n.SetRate(freq);
        }
        for (auto& n : right_noise_) {
            n.SetRate(freq);
        }
    }

    void SetMix(float mix) {
        mix_ = mix;
    }

    void SetMono(bool p) {
        mono_modulator_ = p;
    }

    void SetAltK(bool k) {
        alt_k_ = k;
        _CalcK();
    }
private:
    void _CalcK() {
        float k = used_k_;
        for (size_t i = 0; i < kNumBlock; ++i) {
            float kk = i % 2 == 0 ? k : -k;
            if (!alt_k_) kk = k;
            // left_block_[i].SetK(kk);
            // rigt_block_[i].SetK(kk);
            k_smooth_[i].SetTarget(kk);
        }
    }

    std::array<LatticeBlock, kNumBlock> left_block_;
    std::array<LatticeBlock, kNumBlock> rigt_block_;
    size_t num_block_{};
    // float begin_{};
    // float end_{};
    float fs_{};
    float used_k_{};
    float mix_{};
    bool mono_modulator_{};
    bool alt_k_{};
    dsp::Noise left_noise_[kNumBlock];
    dsp::Noise right_noise_[kNumBlock];
    dsp::ConstantTimeSmoother begin_smooth_;
    dsp::ConstantTimeSmoother end_smooth_;
    dsp::ExpSmoother k_smooth_[kNumBlock];
};
}
#pragma once
#include "delay_line.hpp"
#include "noise.hpp"
#include "thiran_filter.hpp"
#include <cstddef>
#include <array>

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
    static constexpr size_t kNumBlock = 8;

    void Init(float max_ms, float fs) {
        for (auto& b : block_) {
            b.Init(max_ms, fs);
        }
        for (auto& n : noise_) {
            n.Init(fs);
            n.Reset();
        }
        fs_ = fs;
    }

    float Tick(float x) {
        float y = x;
        // update delay time
        for (size_t i = 0; i < num_block_; ++i) {
            block_[i].SetDelay(std::lerp(begin_, end_, noise_[i].Tick()), fs_);
        }

        // process
        for (size_t i = 0; i < num_block_; ++i) {
            x = block_[i].TickUp(x);
        }
        for (int i = num_block_ - 1; i >= 0; --i) {
            x = block_[i].TickDown(x);
        }
        return std::lerp(y, x, mix_);
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
        begin_ = begin;
    }

    void SetDelayEnd(float end) {
        end_ = end;
    }

    void SetFrequency(float freq) {
        for (auto& n : noise_) {
            n.SetRate(freq);
        }
    }

    void SetMix(float mix) {
        mix_ = mix;
    }
private:
    void _CalcK() {
        float max_k = 2.0f / num_block_;
        float k = used_k_;
        if (k > max_k) {
            k = max_k;
        }
        else if (k < -max_k) {
            k = -max_k;
        }
        for (auto& f : block_) {
            f.SetK(k);
        }
    }

    std::array<LatticeBlock, kNumBlock> block_;
    size_t num_block_{};
    float begin_{};
    float end_{};
    float fs_{};
    float used_k_{};
    float mix_{};
    dsp::Noise noise_[kNumBlock];
};
}
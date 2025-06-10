#pragma once
#include <span>
#include <array>

namespace dsp {

class AllpassFilter {
public:
    float Process(float in) {
        auto v = latch_;
        auto t = in - alpha_ * v;
        latch_ = t;
        last_ = v + alpha_ * t;
        return last_;
    }
    void  SetAlpha(float a) { alpha_ = a; }
    float GetLast() const { return last_; }
    void  Reset() {
        latch_ = 0.0f;
        last_ = 0.0f;
    }
private:
    float latch_{};
    float alpha_{ 1.0f };
    float last_{};
};

class BackLPC {
public:
    static constexpr int kNumPoles = 200;
    void Init(float sample_rate);
    void Process(std::span<float> block, std::span<float> block2);
    float ProcessSingle(float x, float exci);

    void SetForget(float forget);
    void SetLearn(float learn) { learn_ = learn; }
    void SetSmooth(float smooth);
    void SetLPCOrder(int order);
    void SetApAlpha(float alpha);
private:
    float sample_rate_{};
    float forget_{};
    float learn_{};
    float smooth_{};
    float gain_latch_{};
    int lpc_order_{};

    std::array<float, kNumPoles> lattice_k_{};
    std::array<float, kNumPoles> iir_k_{};
    std::array<float, kNumPoles> eb_latch_{};
    std::array<float, kNumPoles + 1> eb_out_{};
    std::array<float, kNumPoles + 1> ef_out_{};
    std::array<float, kNumPoles + 1> eb_out_latch_{};
    std::array<float, kNumPoles> ebsum_{};
    std::array<float, kNumPoles> efsum_{};
    std::array<float, kNumPoles + 1> x_iir_{};
    // std::array<float, kNumPoles + 1> l_iir{};
    std::array<AllpassFilter, kNumPoles + 1> iir_latch_{};
};

}

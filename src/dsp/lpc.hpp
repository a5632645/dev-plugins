#pragma once
#include <span>
#include <array>
#include "ExpSmoother.hpp"

namespace dsp {

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
    void SetGainAttack(float ms);
    void SetGainRelease(float ms);

    int GetOrder() const { return lpc_order_; }
    void CopyLatticeCoeffient(std::span<float> buffer);
private:
    ExpSmoother<float> gain_smooth_;

    float sample_rate_{};
    float forget_{};
    float learn_{};
    float smooth_{};
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
    std::array<float, kNumPoles + 1> l_iir{};
};

}

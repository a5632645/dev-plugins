#pragma once
#include <span>
#include <array>
#include "ExpSmoother.hpp"
#include "filter.hpp"

namespace dsp {

class BurgLPC {
public:
    static constexpr int kNumPoles = 80;
    void Init(float sample_rate);
    void Process(std::span<float> block, std::span<float> block2);
    inline float ProcessSingle(float x, float exci);

    void SetForget(float forget);
    void SetSmooth(float smooth);
    void SetLPCOrder(int order);
    void SetGainAttack(float ms);
    void SetGainRelease(float ms);
    void SetDicimate(int dicimate);

    int GetOrder() const { return lpc_order_; }
    void CopyLatticeCoeffient(std::span<float> buffer);
private:
    Filter main_downsample_filter_;
    Filter side_downsample_filter_;
    Filter upsample_filter_;
    float upsample_latch_{};

    ExpSmoother<float> gain_smooth_;
    float gain_attack_{};
    float gain_release_{};
    int dicimate_{};
    int dicimate_counter_{};

    float sample_rate_{};
    float forget_{};
    float forget_ms_{};
    float smooth_{};
    float smooth_ms_{};
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

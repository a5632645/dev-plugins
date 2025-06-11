#pragma once
#include <span>
#include <array>

namespace dsp {
    
class RLSLPC {
public:
    static constexpr int kOrder = 10;

    void Init(float fs);
    void Process(std::span<float> block, std::span<float> block2);
    float ProcessSingle(float x, float exci);
    int GetOrder() const { return kOrder; }
    void CopyLatticeCoeffient(std::span<float> buffer);
private:
    float forget_ = 0.99f;
    std::array<std::array<float, kOrder>, kOrder> p_{};
    std::array<float, kOrder> w_{};
    std::array<float, kOrder> latch_{};
};

} // namespace dsp

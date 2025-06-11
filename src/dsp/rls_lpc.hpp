#pragma once
#include <span>
#include <array>
#include <Eigen/Dense>
#include "ExpSmoother.hpp"

namespace dsp {
    
class RLSLPC {
public:
    static constexpr int kOrder = 35;
    using vec = Eigen::Matrix<double, kOrder, 1>;
    using mat = Eigen::Matrix<double, kOrder, kOrder>;

    void Init(float fs);
    void Process(std::span<float> block, std::span<float> block2);
    float ProcessSingle(float x, float exci);
    int GetOrder() const { return kOrder; }
    void CopyLatticeCoeffient(std::span<float> buffer);
private:
    int num_zero_ = kOrder;

    double forget_ = 0.999f;
    // std::array<std::array<float, kOrder>, kOrder> p_{};
    // std::array<float, kOrder> w_{};
    // std::array<float, kOrder> latch_{};
    mat p_;
    vec w_;
    vec latch_;
    vec k_;

    vec iir_w_;
    vec iir_latch_;
    double smooth_gain_{};
};

} // namespace dsp

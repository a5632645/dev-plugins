#pragma once
#include <span>
#include <array>
#include <Eigen/Dense>
#include "Eigen/Core"
#include "ExpSmoother.hpp"

namespace dsp {
    
class RLSLPC {
public:
    static constexpr int kOrder = 200;
    static constexpr int kDefaultOrder = 35;
    using vec = Eigen::Matrix<double, Eigen::Dynamic, 1>;
    using mat = Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>;

    void Init(float fs);
    void Process(std::span<float> block, std::span<float> block2);
    float ProcessSingle(float x, float exci);
    int GetOrder() const { return order_; }
    void CopyLatticeCoeffient(std::span<float> buffer);

    void SetForgetRate(float ms);
    void SetSmooth(float smooth);
    void SetLPCOrder(int order);
    void SetApAlpha(float alpha);
    void SetGainAttack(float ms);
    void SetGainRelease(float ms);
private:
    int order_{};
    float sample_rate_{};
    double forget_ = 0.999f;
    mat p_;
    mat identity_;
    vec w_;
    vec latch_;
    vec k_;

    vec iir_latch_;
    ExpSmoother<double> gain_smoother_;
};

} // namespace dsp

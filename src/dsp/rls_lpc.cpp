#include "rls_lpc.hpp"
#include "Eigen/Core"
#include <cassert>
#include <cmath>
#include <cstdlib>

namespace dsp {

void RLSLPC::Init(float fs) {
    // for (int i = 0; i < kOrder; ++i) {
    //     p_[i][i] = 0.01f;
    // }

    sample_rate_ = fs;
    forget_ = std::exp(-1.0f / (fs * 20.0f / 1000.0f));
    gain_smoother_.Init(fs);

    p_.resize(kOrder, kOrder);
    identity_.resize(kOrder, kOrder);
    w_.resize(kOrder, Eigen::NoChange);
    latch_.resize(kOrder, Eigen::NoChange);
    k_.resize(kOrder, Eigen::NoChange);
    iir_latch_.resize(kOrder, Eigen::NoChange);
}

void RLSLPC::Process(std::span<float> block, std::span<float> block2) {
    int size = static_cast<int>(block.size());
    for (int i = 0; i < size; ++i) {
        block[i] = ProcessSingle(block[i], block2[i]);
    }
}

float RLSLPC::ProcessSingle(float x, float exci) {
    x += 0.001f * rand() / static_cast<float>(RAND_MAX);

    // prediate
    double pred = w_.transpose() * latch_;
    double lamda_inv = 1.0f / forget_;
    double err = x - pred;

    mat wtf = p_ * lamda_inv;
    k_ = (p_ * latch_) / (forget_ + (latch_.transpose() * p_ * latch_).value());
    w_.noalias() += k_ * err;
    p_.noalias() = (identity_ - k_ * latch_.transpose()) * wtf;

    // push latch
    for (int i = order_ - 1; i > 0; --i) {
        latch_[i] = latch_[i - 1];
    }
    latch_[0] = x;

    // iir processing
    double gain = std::sqrt(err * err + 1e-10f);
    gain = gain_smoother_.Process(gain);

    double yy = w_.transpose() * iir_latch_ + gain * exci;
    yy = std::clamp(yy, -4.0, 4.0);
    for (int i = order_ - 1; i > 0; --i) {
        iir_latch_[i] = iir_latch_[i - 1];
    }
    iir_latch_[0] = yy;

    assert(!isnan(yy));

    return static_cast<float>(yy);
}

void RLSLPC::CopyLatticeCoeffient(std::span<float> buffer) {
    std::copy_n(w_.begin(), order_, buffer.begin());
}

void RLSLPC::SetForgetRate(float ms) {
    forget_ = std::exp(-1.0f / (sample_rate_ * ms / 1000.0f));
}

void RLSLPC::SetSmooth(float smooth) {
}

void RLSLPC::SetLPCOrder(int order) {
    order_ = order;

    p_.resize(order, order);
    identity_.resize(order, order);
    w_.resize(order, Eigen::NoChange);
    latch_.resize(order, Eigen::NoChange);
    k_.resize(order, Eigen::NoChange);
    iir_latch_.resize(order, Eigen::NoChange);

    p_.setIdentity();
    identity_.setIdentity();
    p_.noalias() = identity_ * 0.01;
    w_.setZero();
    latch_.setZero();
    k_.setZero();
    iir_latch_.setZero();
}

void RLSLPC::SetApAlpha(float alpha) {
    // nothing
}

void RLSLPC::SetGainAttack(float ms) {
    gain_smoother_.SetAttackTime(ms);
}

void RLSLPC::SetGainRelease(float ms) {
    gain_smoother_.SetReleaseTime(ms);
}


} // namespace dsp
#include "rls_lpc.hpp"
#include <cassert>
#include <cmath>
#include <cstdlib>

namespace dsp {

void RLSLPC::Init(float fs) {
    // for (int i = 0; i < kOrder; ++i) {
    //     p_[i][i] = 0.01f;
    // }

    forget_ = std::exp(-1.0f / (fs * 20.0f / 1000.0f));

    p_.setIdentity();
    p_ *= 0.01f;
    w_.setZero();
    latch_.setZero();

    iir_latch_.setZero();
    iir_w_.setZero();
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
    float pred = w_.transpose() * latch_;
    float lamda_inv = 1.0f / forget_;
    float err = x - pred;

    mat wtf = p_ * lamda_inv;
    k_ = (p_ * latch_) / (forget_ + (latch_.transpose() * p_ * latch_).value());
    w_.noalias() += k_ * err;
    if (num_zero_ != kOrder) {
        p_.noalias() = (mat::Identity() - k_ * latch_.transpose()) * wtf;
    }

    // push latch
    bool zero_last = std::abs(latch_(kOrder - 1)) < 1e-6;
    bool zero_push = std::abs(x) < 1e-6;
    if (!zero_last && zero_push) ++num_zero_;
    else if (zero_last && !zero_push) --num_zero_;

    for (int i = kOrder - 1; i > 0; --i) {
        latch_[i] = latch_[i - 1];
    }
    latch_[0] = x;

    // iir processing
    float gain = std::sqrt(err * err + 1e-10f);
    iir_w_ = iir_w_ * forget_ + w_ * (1.0f - forget_);
    smooth_gain_ = smooth_gain_ * forget_ + gain * (1.0f - forget_);

    float yy = iir_w_.transpose() * iir_latch_ + smooth_gain_ * exci;
        for (int i = kOrder - 1; i > 0; --i) {
        iir_latch_[i] = iir_latch_[i - 1];
    }
    iir_latch_[0] = yy;

    assert(!isnan(yy));

    return yy;
}

void RLSLPC::CopyLatticeCoeffient(std::span<float> buffer) {
    std::copy_n(w_.begin(), kOrder, buffer.begin());
}
} // namespace dsp
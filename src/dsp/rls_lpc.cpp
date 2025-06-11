#include "rls_lpc.hpp"
#include <cassert>
#include <cmath>

namespace dsp {

void RLSLPC::Init(float fs) {
    // for (int i = 0; i < kOrder; ++i) {
    //     p_[i][i] = 0.01f;
    // }

    forget_ = std::exp(-1.0f / (fs * 10.0f / 1000.0f));

    p_.setIdentity();
    p_ *= 0.01f;
    w_.setZero();
    latch_.setZero();
}

void RLSLPC::Process(std::span<float> block, std::span<float> block2) {
    for (auto& s : block) {
        s = ProcessSingle(s, s);
    }
}

float RLSLPC::ProcessSingle(float x, float exci) {
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
    float yy = w_.transpose() * iir_latch_ + err;
        for (int i = kOrder - 1; i > 0; --i) {
        iir_latch_[i] = iir_latch_[i - 1];
    }
    iir_latch_[0] = yy;

    assert(!isnan(yy));

    return pred;

    // // kalman gain
    // std::array<float, kOrder> k{};
    // for (int i = 0; i < kOrder; ++i) {
    //     for (int j = 0; j < kOrder; ++j) {
    //         k[i] += p_[i][j] * latch_[j];
    //     }
    // }
    // {
    //     float kk = 0.0f;
    //     for (int i = 0; i < kOrder; ++i) {
    //         kk += k[i] * latch_[i];
    //     }
    //     for (int i = 0; i < kOrder; ++i) {
    //         k[i] /= (kk + forget_);
    //     }
    // }

    // // prediate
    // float pred = 0.0f;
    // for (int i = 0; i < kOrder; ++i) {
    //     pred += w_[i] * latch_[i];
    // }
    // float err = x - pred;

    // // update coeffient
    // for (int i = 0; i < kOrder; ++i) {
    //     w_[i] += err * k[i];
    // }

    // // update p
    // if (num_zero_ == 0) {
    //     std::array<std::array<float, kOrder>, kOrder> tmp{};
    //     for (int i = 0; i < kOrder; ++i) {
    //         for (int j = 0; j < kOrder; ++j) {
    //             for (int kk = 0; kk < kOrder; ++kk) {
    //                 tmp[i][j] += p_[j][kk] * k[i] * latch_[kk];
    //             }
    //         }
    //     }
    //     for (int i = 0; i < kOrder; ++i) {
    //         for (int j = 0; j < kOrder; ++j) {
    //             p_[i][j] = (p_[i][j] - tmp[i][j]) / forget_;
    //         }
    //     }
    // }

    // return x;
}

void RLSLPC::CopyLatticeCoeffient(std::span<float> buffer) {
    std::copy_n(w_.begin(), kOrder, buffer.begin());
}
} // namespace dsp
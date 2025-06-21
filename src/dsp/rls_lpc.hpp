#pragma once
#include <cstddef>
#include <span>
#include <cassert>
#include <Eigen/Dense>
#include "Eigen/Core"
#include "ExpSmoother.hpp"
#include "filter.hpp"

namespace dsp {

namespace internal {

template <int LPC_SIZE>
class FIXED_RLSLPC {
public:
    using vec = Eigen::Matrix<double, LPC_SIZE, 1>;
    using mat = Eigen::Matrix<double, LPC_SIZE, LPC_SIZE>;
    static constexpr int order = LPC_SIZE;

    void Init(float fs);
    void Process(std::span<float> block, std::span<float> block2);
    float ProcessSingle(float x, float exci);
    constexpr int GetOrder() const { return LPC_SIZE; }
    void CopyTransferFunction(std::span<float> buffer);

    void SetForgetParam(float forget);
    void SetLPCOrder(int order);
    void SetGainAttack(float ms);
    void SetGainRelease(float ms);
private:
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

template <int LPC_SIZE>
void FIXED_RLSLPC<LPC_SIZE>::Init(float fs) {
    sample_rate_ = fs;
    forget_ = std::exp(-1.0f / (fs * 20.0f / 1000.0f));
    gain_smoother_.Init(fs);

    identity_.setIdentity();
    p_.noalias() = identity_ * 0.01;
    w_.setZero();
    latch_.setZero();
    iir_latch_.setZero();
}

template <int LPC_SIZE>
void FIXED_RLSLPC<LPC_SIZE>::Process(std::span<float> block, std::span<float> block2) {
    int size = static_cast<int>(block.size());
    for (int i = 0; i < size; ++i) {
        block[i] = ProcessSingle(block[i], block2[i]);
    }
}

template <int LPC_SIZE>
float FIXED_RLSLPC<LPC_SIZE>::ProcessSingle(float x, float exci) {
    float noise = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX) - 0.5f) * 1e-4f;
    x += noise;

    // prediate
    double pred = w_.transpose() * latch_;
    double lamda_inv = 1.0f / forget_;
    double err = x - pred;

    auto wtf = p_ * lamda_inv;
    k_.noalias() = (p_ * latch_) / (forget_ + (latch_.transpose() * p_ * latch_).value());
    w_.noalias() += k_ * err;
    p_ = (identity_ - k_ * latch_.transpose()) * wtf;

    // push latch
    for (int i = LPC_SIZE - 1; i > 0; --i) {
        latch_[i] = latch_[i - 1];
    }
    latch_[0] = x;

    // iir processing
    double gain = std::sqrt(err * err + 1e-10f);
    gain = gain_smoother_.Process(gain);

    double yy = w_.transpose() * iir_latch_ + gain * exci;
    yy = std::clamp(yy, -4.0, 4.0);
    for (int i = LPC_SIZE - 1; i > 0; --i) {
        iir_latch_[i] = iir_latch_[i - 1];
    }
    iir_latch_[0] = yy;

    assert(!isnan(yy));

    return static_cast<float>(yy);
}

template <int LPC_SIZE>
void FIXED_RLSLPC<LPC_SIZE>::CopyTransferFunction(std::span<float> buffer) {
    std::copy_n(w_.begin(), LPC_SIZE, buffer.begin());
}

template <int LPC_SIZE>
void FIXED_RLSLPC<LPC_SIZE>::SetForgetParam(float forget) {
    forget_ = forget;
}

template <int LPC_SIZE>
void FIXED_RLSLPC<LPC_SIZE>::SetGainAttack(float ms) {
    gain_smoother_.SetAttackTime(ms);
}

template <int LPC_SIZE>
void FIXED_RLSLPC<LPC_SIZE>::SetGainRelease(float ms) {
    gain_smoother_.SetReleaseTime(ms);
}

} // namespace internal

// --------------------------------------------------------------------------------
// RLS LPC
// --------------------------------------------------------------------------------
class RLSLPC {
public:
    static constexpr int kNumLPC = 6;
    static constexpr std::array kLPCOrders {
        8, 10, 15, 20, 30, 40
    };

    void Init(float fs);
    void Process(std::span<float> block, std::span<float> block2);
    int GetOrder() const { return order_; }
    void CopyTransferFunction(std::span<float> buffer);

    void SetForgetRate(float ms);
    void SetGainAttack(float ms);
    void SetGainRelease(float ms);
    void SetOrder(int order);
    void SetDicimate(int dicimate);
private:
    template<int LPC_SIZE>
    void ProcessBlock(internal::FIXED_RLSLPC<LPC_SIZE>& lpc, std::span<float> block, std::span<float> block2) {
        for (size_t i = 0; i < block.size(); ++i) {
            float filt_x = main_downsample_filter_.ProcessSingle(block[i]);
            float filt_side = side_downsample_filter_.ProcessSingle(block2[i]);
            ++dicimate_counter_;
            if (dicimate_counter_ > dicimate_) {
                dicimate_counter_ = 1;

                upsample_latch_ = lpc.ProcessSingle(filt_x, filt_side);
            }
            block[i] = upsample_filter_.ProcessSingle(upsample_latch_);
        }
    }

    internal::FIXED_RLSLPC<8> lpc8_;
    internal::FIXED_RLSLPC<10> lpc10_;
    internal::FIXED_RLSLPC<15> lpc15_;
    internal::FIXED_RLSLPC<20> lpc20_;
    internal::FIXED_RLSLPC<30> lpc35_;
    internal::FIXED_RLSLPC<40> lpc40_;
    int order_{};
    Filter main_downsample_filter_;
    Filter side_downsample_filter_;
    Filter upsample_filter_;
    float upsample_latch_ = 0.0f;
    int dicimate_ = 1;
    int dicimate_counter_ = 1;
    float forget_ms_{};
    float sample_rate_{};
};

} // namespace dsp

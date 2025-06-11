#include "rls_lpc.hpp"
#include <cassert>

namespace dsp {

void RLSLPC::Init(float fs) {
    for (int i = 0; i < kOrder; ++i) {
        p_[i][i] = 0.01f;
    }
}

void RLSLPC::Process(std::span<float> block, std::span<float> block2) {
    for (auto& s : block) {
        s = ProcessSingle(s, s);
    }
}

float RLSLPC::ProcessSingle(float x, float exci) {    
    // kalman gain
    std::array<float, kOrder> k{};
    for (int i = 0; i < kOrder; ++i) {
        for (int j = 0; j < kOrder; ++j) {
            k[i] += p_[i][j] * latch_[j];
        }
    }
    {
        float kk = 0.0f;
        for (int i = 0; i < kOrder; ++i) {
            kk += k[i] * latch_[i];
        }
        for (int i = 0; i < kOrder; ++i) {
            k[i] /= (kk + forget_);
        }
    }

    // prediate
    float pred = 0.0f;
    for (int i = 0; i < kOrder; ++i) {
        pred += w_[i] * latch_[i];
    }
    float err = x - pred;

    // update coeffient
    for (int i = 0; i < kOrder; ++i) {
        w_[i] += err * k[i];
    }

    // update p
    std::array<std::array<float, kOrder>, kOrder> tmp{};
    for (int i = 0; i < kOrder; ++i) {
        for (int j = 0; j < kOrder; ++j) {
            for (int kk = 0; kk < kOrder; ++kk) {
                tmp[i][j] += p_[j][kk] * k[i] * latch_[kk];
            }
        }
    }
    for (int i = 0; i < kOrder; ++i) {
        for (int j = 0; j < kOrder; ++j) {
            p_[i][j] = (p_[i][j] - tmp[i][j]) / forget_;
        }
    }

    // push latch
    for (int i = kOrder - 1; i > 0; --i) {
        latch_[i] = latch_[i - 1];
    }
    latch_[0] = x;

    return x;
}

void RLSLPC::CopyLatticeCoeffient(std::span<float> buffer) {
    std::copy_n(w_.begin(), kOrder, buffer.begin());
}

}
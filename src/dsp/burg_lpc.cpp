#include "burg_lpc.hpp"
#include <cassert>
#include <cmath>
#include <cstdlib>

namespace dsp {
void BurgLPC::Init(float sample_rate) {
    sample_rate_ = sample_rate;
    gain_smooth_.Init(sample_rate);
    main_downsample_filter_.Init(sample_rate);
    side_downsample_filter_.Init(sample_rate);
    upsample_filter_.Init(sample_rate);
}

void BurgLPC::Process(std::span<float> block, std::span<float> block2) {
    assert(block2.size() == block.size());
    for (size_t i = 0; i < block.size(); ++i) {
        block[i] = ProcessSingle(block[i], block2[i]);
    }
}

float BurgLPC::ProcessSingle(float x, float exci) {
    float filt_x = main_downsample_filter_.ProcessSingle(x);
    float filt_side = side_downsample_filter_.ProcessSingle(exci);
    
    ++dicimate_counter_;
    if (dicimate_counter_ > dicimate_) {
        dicimate_counter_ = 1;
        float noise = static_cast<float>(rand()) * 1e-4f / static_cast<float>(RAND_MAX);
        filt_x += noise;

        ef_out_[0] = filt_x;
        eb_out_[0] = filt_x;
        for (int i = 0; i < lpc_order_; ++i) {
            float up = ef_out_[i] * eb_out_latch_[i];
            float down = ef_out_[i] * ef_out_[i] + eb_out_latch_[i] * eb_out_latch_[i];
            efsum_[i] = forget_ * efsum_[i] + up * learn_;
            ebsum_[i] = forget_ * ebsum_[i] + down * learn_;
            lattice_k_[i] = -2.0f * efsum_[i] / ebsum_[i];
            if (lattice_k_[i] >= 1.0f - 1e-6f) lattice_k_[i] = 1.0f - 1e-6f;
            else if (lattice_k_[i] < -(1.0f - 1e-6f)) lattice_k_[i] = -(1.0f - 1e-6f);
            ef_out_[i + 1] = ef_out_[i] + lattice_k_[i] * eb_latch_[i];
            eb_out_[i + 1] = eb_latch_[i] + lattice_k_[i] * ef_out_[i];
            eb_latch_[i] = eb_out_[i];
            eb_out_latch_[i] = eb_out_[i];
        }

        // TODO: interpolate filter
        for (int i = 0; i < lpc_order_; ++i) {
            iir_k_[i] = iir_k_[i] * smooth_ + lattice_k_[i] * (1.0f - smooth_);
        }

        // iir part
        float recusial = ef_out_[lpc_order_];
        float gain = std::sqrt(recusial * recusial + 1e-20f);
        gain = gain_smooth_.Process(gain);
        x_iir_[0] = filt_side * gain;
        // iir lattice
        for (int i = 0; i < lpc_order_; ++i) {
            x_iir_[i + 1] = x_iir_[i] - iir_k_[lpc_order_ - i - 1] * l_iir[i + 1];
        }
        for (int i = 0; i < lpc_order_; ++i) {
            l_iir[i] = l_iir[i + 1] + iir_k_[lpc_order_ - i - 1] * x_iir_[i + 1];
        }
        l_iir[lpc_order_] = x_iir_[lpc_order_];

        upsample_latch_ = x_iir_[lpc_order_];
    }

    float out = upsample_filter_.ProcessSingle(upsample_latch_);
    return out;
}

void BurgLPC::SetSmooth(float smooth) {
    smooth_ = std::exp(-1.0f / (sample_rate_ * smooth / 1000.0f));
}

void BurgLPC::SetForget(float forget_ms) {
    forget_ms_ = forget_ms;
    forget_ = std::exp(-1.0f / ((sample_rate_ / dicimate_) * forget_ms / 1000.0f));
}

void BurgLPC::SetLPCOrder(int order) {
    lpc_order_ = order;
    std::fill_n(lattice_k_.begin(), order, 0.0f);
    std::fill_n(iir_k_.begin(), order, 0.0f);
    std::fill_n(eb_latch_.begin(), order, 0.0f);
    std::fill_n(eb_out_.begin(), order + 1, 0.0f);
    std::fill_n(ef_out_.begin(), order + 1, 0.0f);
    std::fill_n(eb_out_latch_.begin(), order + 1, 0.0f);
    std::fill_n(ebsum_.begin(), order, 0.0f);
    std::fill_n(efsum_.begin(), order, 0.0f);
    std::fill_n(x_iir_.begin(), order + 1, 0.0f);
    std::fill_n(l_iir.begin(), order + 1, 0.0f);
}

void BurgLPC::SetGainAttack(float ms) {
    gain_smooth_.SetAttackTime(ms);
}

void BurgLPC::SetGainRelease(float ms) {
    gain_smooth_.SetReleaseTime(ms);
}

void BurgLPC::CopyLatticeCoeffient(std::span<float> buffer) {
    std::copy_n(iir_k_.begin(), lpc_order_, buffer.begin());
}

void BurgLPC::SetDicimate(int dicimate) {
    dicimate_ = dicimate;
    dicimate_counter_ = dicimate + 1;
    main_downsample_filter_.MakeDownSample(dicimate);
    side_downsample_filter_.MakeDownSample(dicimate);
    upsample_filter_.MakeDownSample(dicimate);
    upsample_latch_ = 0.0f;
    SetForget(forget_ms_);
}

}
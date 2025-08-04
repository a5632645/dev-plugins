#include "rls_lpc.hpp"
#include <cassert>
#include <cstddef>
#include <cstdlib>

namespace dsp {

void RLSLPC::Init(float fs) {
    sample_rate_ = fs;
    lpc8_.Init(fs);
    lpc16_.Init(fs);
    lpc24_.Init(fs);
    lpc32_.Init(fs);
    lpc40_.Init(fs);
    main_downsample_filter_.Init(fs);
    side_downsample_filter_.Init(fs);
    upsample_filter_.Init(fs);
}

void RLSLPC::Process(std::span<float> block, std::span<float> block2) {
    switch (order_) {
    case lpc8_.order:
        ProcessBlock(lpc8_, block, block2);
        break;
    case lpc16_.order:
        ProcessBlock(lpc16_, block, block2);
        break;
    case lpc24_.order:
        ProcessBlock(lpc24_, block, block2);
        break;
    case lpc32_.order:
        ProcessBlock(lpc32_, block, block2);
        break;
    case lpc40_.order:
        ProcessBlock(lpc40_, block, block2);
        break;
    default:
        assert(false);
        break;
    }
}

void RLSLPC::CopyTransferFunction(std::span<float> buffer) {
    assert(buffer.size() >= static_cast<size_t>(order_));
    switch (order_) {
    case lpc8_.order:
        lpc8_.CopyTransferFunction(buffer);
        break;
    case lpc16_.order:
        lpc16_.CopyTransferFunction(buffer);
        break;
    case lpc24_.order:
        lpc24_.CopyTransferFunction(buffer);
        break;
    case lpc32_.order:
        lpc32_.CopyTransferFunction(buffer);
        break;
    case lpc40_.order:
        lpc40_.CopyTransferFunction(buffer);
        break;
    default:
        assert(false);
        break;
    }
}

void RLSLPC::SetForgetRate(float ms) {
    forget_ms_ = ms;
    float forget_ = std::exp(-1.0f / ((sample_rate_ / dicimate_) * ms / 1000.0f));
    lpc8_.SetForgetParam(forget_);
    lpc16_.SetForgetParam(forget_);
    lpc24_.SetForgetParam(forget_);
    lpc32_.SetForgetParam(forget_);
    lpc40_.SetForgetParam(forget_);
}

void RLSLPC::SetGainAttack(float ms) {
    lpc8_.SetGainAttack(ms);
    lpc16_.SetGainAttack(ms);
    lpc24_.SetGainAttack(ms);
    lpc32_.SetGainAttack(ms);
    lpc40_.SetGainAttack(ms);
}

void RLSLPC::SetGainRelease(float ms) {
    lpc8_.SetGainRelease(ms);
    lpc16_.SetGainRelease(ms);
    lpc24_.SetGainRelease(ms);
    lpc32_.SetGainRelease(ms);
    lpc40_.SetGainRelease(ms);
}

void RLSLPC::SetOrder(int order) {
    if (order >= lpc40_.order) {
        order_ = lpc40_.order;
    }
    else if (order >= lpc40_.order) {
        order_ = lpc40_.order;
    }
    else if (order >= lpc32_.order) {
        order_ = lpc32_.order;
    }
    else if (order >= lpc24_.order) {
        order_ = lpc24_.order;
    }
    else if (order >= lpc16_.order) {
        order_ = lpc16_.order;
    }
    else {
        order_ = lpc8_.order;
    }
}

void RLSLPC::SetDicimate(int dicimate) {
    dicimate_ = dicimate;
    dicimate_counter_ = dicimate + 1;
    upsample_filter_.MakeDownSample(dicimate);
    main_downsample_filter_.MakeDownSample(dicimate);
    side_downsample_filter_.MakeDownSample(dicimate);
    upsample_latch_ = 0.0f;
    SetForgetRate(forget_ms_);
}

} // namespace dsp
#include "rls_lpc.hpp"
#include "Eigen/Core"
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdlib>

namespace dsp {

void RLSLPC::Init(float fs) {
    lpc8_.Init(fs);
    lpc10_.Init(fs);
    lpc15_.Init(fs);
    lpc20_.Init(fs);
    lpc35_.Init(fs);
    lpc40_.Init(fs);
    main_downsample_filter_.Init(fs);
    side_downsample_filter_.Init(fs);
}

void RLSLPC::Process(std::span<float> block, std::span<float> block2) {
    switch (order_) {
    case lpc8_.order:
        lpc8_.Process(block, block2);
        break;
    case lpc10_.order:
        lpc10_.Process(block, block2);
        break;
    case lpc15_.order:
        lpc15_.Process(block, block2);
        break;
    case lpc20_.order:
        lpc20_.Process(block, block2);
        break;
    case lpc35_.order:
        lpc35_.Process(block, block2);
        break;
    case lpc40_.order:
        lpc40_.Process(block, block2);
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
    case lpc10_.order:
        lpc10_.CopyTransferFunction(buffer);
        break;
    case lpc15_.order:
        lpc15_.CopyTransferFunction(buffer);
        break;
    case lpc20_.order:
        lpc20_.CopyTransferFunction(buffer);
        break;
    case lpc35_.order:
        lpc35_.CopyTransferFunction(buffer);
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
    lpc8_.SetForgetRate(ms);
    lpc10_.SetForgetRate(ms);
    lpc15_.SetForgetRate(ms);
    lpc20_.SetForgetRate(ms);
    lpc35_.SetForgetRate(ms);
    lpc40_.SetForgetRate(ms);
}

void RLSLPC::SetGainAttack(float ms) {
    lpc8_.SetGainAttack(ms);
    lpc10_.SetGainAttack(ms);
    lpc15_.SetGainAttack(ms);
    lpc20_.SetGainAttack(ms);
    lpc35_.SetGainAttack(ms);
    lpc40_.SetGainAttack(ms);
}

void RLSLPC::SetGainRelease(float ms) {
    lpc8_.SetGainRelease(ms);
    lpc10_.SetGainRelease(ms);
    lpc15_.SetGainRelease(ms);
    lpc20_.SetGainRelease(ms);
    lpc35_.SetGainRelease(ms);
    lpc40_.SetGainRelease(ms);
}

void RLSLPC::SetOrder(int order) {
    if (order >= lpc40_.order) {
        order_ = lpc40_.order;
    }
    else if (order >= lpc35_.order) {
        order_ = lpc35_.order;
    }
    else if (order >= lpc20_.order) {
        order_ = lpc20_.order;
    }
    else if (order >= lpc15_.order) {
        order_ = lpc15_.order;
    }
    else if (order >= lpc10_.order) {
        order_ = lpc10_.order;
    }
    else {
        order_ = lpc8_.order;
    }
}

} // namespace dsp
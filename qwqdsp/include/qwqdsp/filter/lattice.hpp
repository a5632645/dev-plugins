#pragma once
#include <cassert>
#include <cstddef>
#include <utility>
#include "int_delay.hpp"

namespace qwqdsp_filter {
class LatticeZero {
public:
    void Reset() noexcept {
        latch_ = 0;
    }

    /**
     * @return {up, down} or {min_phase, max_phase}
     */
    std::pair<float, float> Tick(float up, float down) noexcept {
        float const minphase = up + k_ * latch_;
        float const maxphase = up * k_ + latch_;
        latch_ = down;
        return {minphase, maxphase};
    }

    void SetReflection(float k) noexcept {
        k_ = k;
    }
private:
    float latch_{};
    float k_{};
};

class LatticeZeroPolyphase {
public:
    void Init(size_t max_samples) {
        delay_.Init(max_samples);
    }

    void Reset() noexcept {
        delay_.Reset();
    }

    /**
     * @return {up, down} or {min_phase, max_phase}
     */
    std::pair<float, float> Tick(float up, float down) noexcept {
        float const la = delay_.GetBeforePush(n_latch_);
        float const minphase = up + k_ * la;
        float const maxphase = up * k_ + la;
        delay_.Push(down);
        return {minphase, maxphase};
    }

    void SetReflection(float k) noexcept {
        k_ = k;
    }

    void SetNLatch(size_t n) noexcept {
        assert(n != 0);
        n_latch_ = n;
    }
private:
    size_t n_latch_{1};
    float k_{};
    IntDelay delay_;
};

class LatticePole {
public:
    void Reset() noexcept {
        lag_ = 0;
    }

    /**
     * @return {up, down}
     */
    std::pair<float, float> Tick(float up, float down) noexcept {
        auto upgo = up - k_ * lag_;
        auto downgo = lag_ + up * k_;
        lag_ = down;
        return {upgo, downgo};
    }

    void SetReflection(float k) noexcept {
        k_ = k;
    }
private:
    float k_{};
    float lag_{};
};

class LatticePolePolyphase {
public:
    void Init(size_t max_samples) {
        delay_.Init(max_samples);
    }

    void Reset() noexcept {
        delay_.Reset();
    }

    std::pair<float, float> Tick(float up, float down) noexcept {
        auto latch = delay_.GetBeforePush(n_latch_);
        auto upgo = up - k_ * latch;
        auto downgo = latch + up * k_;
        delay_.Push(down);
        return {upgo, downgo};
    }

    void SetReflection(float k) noexcept {
        k_ = k;
    }

    void SetNLatch(size_t n) noexcept {
        assert(n != 0);
        n_latch_ = n;
    }
private:
    float k_{};
    size_t n_latch_{1};
    IntDelay delay_;
};
}

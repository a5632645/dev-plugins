#pragma once
#include <cassert>
#include <cstddef>
#include "int_delay.hpp"

namespace qwqdsp::filter {
class LatticePole {
public:
    void Deinit() {
        latch_ = 0;
    }

    float Tickup(float x) {
        up_going_ = x + k_ * latch_;
        return up_going_;
    }

    float Tickdown(float x) {
        auto y = latch_ - k_ * up_going_;
        latch_ = x;
        return y;
    }

    void SetReflection(float k) {
        k_ = k;
    }
private:
    float k_{};
    float latch_{};
    float up_going_{};
};

class LatticePolePolyphase {
public:
    void Init(size_t max_samples) {
        delay_.Init(max_samples);
    }

    void Deinit() {
        delay_.Deinit();
    }

    float Tickup(float x) {
        latch_ = delay_.GetBeforePush(n_latch_);
        up_going_ = x + k_ * latch_;
        return up_going_;
    }

    float Tickdown(float x) {
        auto y = latch_ - k_ * up_going_;
        delay_.Push(x);
        return y;
    }

    void SetReflection(float k) {
        k_ = k;
    }

    void SetNLatch(size_t n) {
        assert(n != 0);
        n_latch_ = n;
    }
private:
    float k_{};
    float up_going_{};
    float latch_{};
    size_t n_latch_{1};
    IntDelay delay_;
};
}
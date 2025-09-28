#pragma once
#include <algorithm>
#include <cmath>

namespace qwqdsp::misc {
template <bool kUseClamp = true>
class Integrator {
public:
    void Reset() noexcept {
        latch_ = 0.0f;
    }

    float Tick(float x) noexcept {
        if constexpr (kUseClamp) {
            latch_ = std::clamp(latch_ + x, -10.0f, 10.0f);
        }
        else {
            latch_ += x;
        }
        return latch_;
    }
private:
    float latch_{};
};

template<class T, T kLeak = T(0.997)>
class IntegratorLeak {
public:
    void Reset() noexcept {
        sum_ = 0;
    }

    T Tick(T const x) noexcept {
        sum_ *= kLeak;
        sum_ += x;
        return sum_;
    }

    T Gain(T const w) const noexcept {
        T const g2 = 1 + kLeak * kLeak - 2 * kLeak * std::cos(w);
        return std::sqrt(g2);
    }
private:
    T sum_{};
};

template <bool kUseClamp = true>
class IntegratorTrapezoidal {
public:
    void Reset() noexcept {
        sum_ = 0;
        latch_ = 0;
    }

    float Tick(float x) noexcept {
        float const t = (x + latch_) * 0.5f;
        latch_ = x;
        if constexpr (kUseClamp) {
            sum_ = std::clamp(sum_ + t, -10.0f, 10.0f);
        }
        else {
            sum_ += t;
        }
        return sum_;
    }
private:
    float sum_{};
    float latch_{};
};

template <class T, T kLeak = T(0.997)>
class IntegratorTrapezoidalLeak {
public:
    void Reset() noexcept {
        sum_ = 0;
        latch_ = 0;
    }

    T Tick(T x) noexcept {
        T const t = (x + latch_) / 2;
        latch_ = x;
        sum_ *= kLeak;
        sum_ += t;
        return sum_;
    }

    T Gain(T const w) const noexcept {
        T const down = 1 + kLeak * kLeak - 2 * kLeak * std::cos(w);
        T const up = 1 + std::cos(w);
        return std::sqrt(down / up);
    }
private:
    T sum_{};
    T latch_{};
};
}
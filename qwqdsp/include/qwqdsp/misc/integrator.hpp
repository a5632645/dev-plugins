#pragma once
#include <algorithm>
#include <cmath>

namespace qwqdsp_misc {
template <bool kUseClamp = true>
class IntegratorNaive {
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
class IntegratorNaiveLeak {
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

template<class T>
class IntegratorTrapezoidal {
public:
    void Reset() noexcept {
        lag_ = 0;
    }

    T Tick(float x) noexcept {
        x *= T(0.5);
        float const y = x + lag_;
        lag_ = x + y;
        return y;
    }
private:
    T lag_{};
};

template <class T, T kLeak = T(0.997)>
class IntegratorTrapezoidalLeak {
public:
    void Reset() noexcept {
        lag_ = 0;
    }

    T Tick(T x) noexcept {
        T const x_prime = x * T(0.5); 
        lag_ *= kLeak; 
        T const y = x_prime + lag_;
        lag_ += x_prime; 
        return y;
    }

    T Gain(T const w) const noexcept {
        T const down = 1 + kLeak * kLeak - 2 * kLeak * std::cos(w);
        T const up = 1 + std::cos(w);
        return std::sqrt(down / up);
    }
private:
    T lag_{};
};
}

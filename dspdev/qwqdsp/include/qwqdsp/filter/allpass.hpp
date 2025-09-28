#pragma once
#include <complex>
#include <cassert>
#include <cstddef>
#include "int_delay.hpp"

namespace qwqdsp::filter {
/**
 *           a + z^-1
 * H(z) = -------------
 *         1 + a*z^-1
 * @ref http://www.ee.ic.ac.uk/hp/staff/dmb/courses/DSPDF/01000_Structures.pdf
 */
class AllpassOrder1 {
public:
    void Reset() noexcept {
        xlatch_ = 0;
        ylatch_ = 0;
    }

    float Tick(float x) noexcept {
        auto y = xlatch_ + a_ * (x - ylatch_);
        xlatch_ = x;
        ylatch_ = y;
        return y;
    }

    /**
     * @param a [-1, 1]
     */
    void SetA(float a) noexcept {
        a_ = a;
    }

    std::complex<float> GetResponce(std::complex<float> z) noexcept {
        auto up = a_ * z + 1.0f;
        auto down = z + a_;
        return up / down;
    }
private:
    float xlatch_{};
    float ylatch_{};
    float a_{};
};

/**
 *           a2 + a1 * z^-1  + z^-2
 * H(z) = ---------------------------------
 *            1 + a1 * z^-1 + a2 * z^-2
 * @ref http://www.ee.ic.ac.uk/hp/staff/dmb/courses/DSPDF/01000_Structures.pdf
 */
class AllpassOrder2 {
public:
    void Reset() noexcept {
        latch1_ = 0;
        latch2_ = 0;
    }

    float Tick(float x) noexcept {
        auto t = a1_ * latch1_ + a2_ * (x + latch2_);
        auto y = latch2_ + t;
        latch2_ = latch1_;
        latch1_ = x - t;
        return y;
    }

    void SetA1(float a) noexcept {
        a1_ = a;
    }

    void SetA2(float a) noexcept {
        a2_ = a;
    }

    /**
     * @param w [0, pi]
     * @param radius [0, 1]
     */
    void SetPole(float w, float radius) noexcept {
        a1_ = -2.0f * radius * std::cos(w);
        a2_ = radius * radius;
    }

    std::complex<float> GetResponce(std::complex<float> z) noexcept {
        auto z2 = z * z;
        auto up = a2_ * z2 + a1_ * z + 1.0f;
        auto down = z2 + a1_ * z + a2_;
        return up / down;
    }
private:
    float latch1_{};
    float latch2_{};
    float a1_{};
    float a2_{};
};

/**
 * @brief Schroeder allpass
 */
class AllpassPolyphase {
public:
    void Init(size_t max_samples) {
        buffer_.Init(max_samples);
    }

    void Reset() noexcept {
        buffer_.Reset();
    }

    float Tick(float x) noexcept {
        float z1 = buffer_.GetBeforePush(n_latch_);
        float in = x + alpha_ * z1;
        float out = -alpha_ * in + z1;
        buffer_.Push(in);
        return out;
    }

    void SetAlpha(float a) noexcept {
        alpha_ = a;
    }

    float GetAlpha() const noexcept {
        return alpha_;
    }

    void SetNLatch(size_t n) noexcept {
        assert(n != 0);
        n_latch_ = n;
    }

    size_t GetNLatch() const noexcept {
        return n_latch_;
    }

    std::complex<float> GetResponce(std::complex<float> z) const noexcept {
        auto zpow = std::pow(z, -static_cast<float>(n_latch_));
        auto up = zpow - alpha_;
        auto down = 1.0f - alpha_ * zpow;
        return up / down;
    }
private:
    float alpha_{};
    IntDelay buffer_;
    size_t n_latch_{1};
};

/**
 *           a + z^-n
 * H(z) = -------------
 *         1 + a*z^-n
 * @ref http://www.ee.ic.ac.uk/hp/staff/dmb/courses/DSPDF/01000_Structures.pdf
 */
class AllpassOrder1Polyphase {
public:
    void Init(size_t max_samples) {
        xlatch_.Init(max_samples);
        ylatch_.Init(max_samples);
    }

    void Reset() noexcept {
        xlatch_.Reset();
        ylatch_.Reset();
    }

    float Tick(float x) noexcept {
        auto y = xlatch_.GetBeforePush(n_latch_) + a_ * (x - ylatch_.GetBeforePush(n_latch_));
        xlatch_.Push(x);
        ylatch_.Push(y);
        return y;
    }

    /**
     * @param a [-1, 1]
     */
    void SetA(float a) noexcept {
        a_ = a;
    }

    void SetNLatch(size_t n) noexcept {
        assert(n != 0);
        n_latch_ = n;
    }

    std::complex<float> GetResponce(std::complex<float> z) noexcept {
        auto const powz = std::pow(z, static_cast<float>(n_latch_));
        auto const up = a_ * powz + 1.0f;
        auto const down = powz + a_;
        return up / down;
    }
private:
    IntDelay xlatch_;
    IntDelay ylatch_;
    size_t n_latch_{1};
    float a_{};
};

/**
 *           a2 + a1 * z^-n  + z^-2n
 * H(z) = ---------------------------------
 *            1 + a1 * z^-n + a2 * z^-2n
 * @ref http://www.ee.ic.ac.uk/hp/staff/dmb/courses/DSPDF/01000_Structures.pdf
 */
class AllpassOrder2Polyphase {
public:
    void Init(size_t max_samples) {
        latch1_.Init(max_samples);
        latch2_.Init(max_samples);
    }

    void Reset() noexcept {
        latch1_.Reset();
        latch2_.Reset();
    }

    float Tick(float x) noexcept {
        auto const latch1 = latch1_.GetBeforePush(n_latch_);
        auto const latch2 = latch2_.GetBeforePush(n_latch_);
        auto const t = a1_ * latch1 + a2_ * (x + latch2);
        auto const y = latch2 + t;
        latch2_.Push(latch1);
        latch1_.Push(x - t);
        return y;
    }

    void SetA1(float a) noexcept {
        a1_ = a;
    }

    void SetA2(float a) noexcept {
        a2_ = a;
    }

    /**
     * @param w [0, pi]
     * @param radius [0, 1]
     */
    void SetPole(float w, float radius) noexcept {
        a1_ = -2.0f * radius * std::cos(w);
        a2_ = radius * radius;
    }

    void SetNLatch(size_t n) noexcept {
        assert(n != 0);
        n_latch_ = n;
    }

    std::complex<float> GetResponce(std::complex<float> z) noexcept {
        auto const powz = std::pow(z, static_cast<float>(n_latch_));
        auto const z2 = powz * powz;
        auto const up = a2_ * z2 +a1_ * powz + 1.0f;
        auto const down = z2 + a1_ * powz + a2_;
        return up / down;
    }
private:
    IntDelay latch1_;
    IntDelay latch2_;
    size_t n_latch_{1};
    float a1_{};
    float a2_{};
};
}
#pragma  once
#include <cstddef>
#include <array>
#include <complex>
#include <cmath>
#include <numbers>

namespace qwqdsp::filter {
/**
 * @brief full iir hilbert
 * @ref https://dsp.stackexchange.com/questions/8692/hilbert-transform-filter-for-audio-applications-using-iir-half-band-parallel-al/24732#24732
 */
template<size_t kNumFilter>
class IIRHilbertFull {
public:
    void Reset() noexcept {
        latch_ = 0;
        for (auto& f : real_) {
            f.Reset();
        }
        for (auto& f : imag_) {
            f.Reset();
        }
    }

    IIRHilbertFull() noexcept {
        for (size_t i = 0; i < kNumFilter; ++i) {
            float preal = std::exp(-std::numbers::pi_v<float> / std::exp2(2.0f * i));
            float pimag = std::exp(-std::numbers::pi_v<float> / std::exp2(2.0f * i + 1.0f));
            real_[i].alpha_ = preal;
            imag_[i].alpha_ = pimag;
        }
    }

    std::complex<float> Tick(float x) noexcept {
        float real = x;
        float imag = latch_;
        for (auto& rf : real_) {
            real = rf.Tick(real);
        }
        latch_ = x;
        for (auto& f : imag_) {
            latch_ = f.Tick(latch_);
        }
        return {real, imag};
    }
private:
    struct APF {
        float z0_{};
        float z1_{};
        float alpha_{};

        void Reset() noexcept {
            z0_ = 0;
            z1_ = 0;
        }

        float Tick(float x) noexcept {
            float in = x + alpha_ * z1_;
            float out = -alpha_ * in + z1_;
            z1_ = z0_;
            z0_ = in;
            return out;
        }
    };
    float latch_{};
    std::array<APF, kNumFilter> real_;
    std::array<APF, kNumFilter> imag_;
};
}
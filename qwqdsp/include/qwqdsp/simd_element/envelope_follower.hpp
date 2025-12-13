#pragma once
#include "simd_pack.hpp"
#include "qwqdsp/misc/smoother.hpp"

namespace qwqdsp_simd_element {
template<size_t N>
class EnevelopeFollower {
public:
    void Reset() noexcept {
        peakhold_.Reset();
        smoother_.Reset();
    }

    PackFloat<N> Tick(PackFloat<N> x) noexcept {
        x = PackOps::Abs(x);
        x = peakhold_.Tick(x);
        return smoother_.Tick(x);
    }

    void SetAttackTime(float ms, float fs) noexcept {
        smoother_.SetAttackTime(ms, fs);
    }

    void SetReleaseTime(float ms, float fs) noexcept {
        smoother_.SetReleaseTime(ms, fs);
    }

    void SetHoldTime(float ms, float fs) noexcept {
        peakhold_.SetHoldSamples(static_cast<int>(ms * fs / 1000.0f));
    }
private:
    class SimplePeakHold {
    public:
        void Reset() noexcept {
            lag_ = 0;
            counter_ = 0;
        }

        PackFloat<N> Tick(PackFloatCRef<N> x) noexcept {
            for (size_t i = 0; i < N; ++i) {
                --counter_[i];
                if (x[i] > lag_[i]) {
                    lag_[i] = x[i];
                    counter_[i] = hold_samples_;
                }
                if (counter_[i] < 0) {
                    lag_[i] = x[i];
                    counter_[i] = hold_samples_;
                }
            }
            return lag_;
        }

        void SetHoldSamples(int samples) noexcept {
            hold_samples_ = samples;
            counter_ = PackOps::Min(counter_, PackInt32<N>::vBroadcast(samples));
        }
    private:
        PackFloat<N> lag_{0.0f};
        int hold_samples_{};
        PackInt32<N> counter_{};
    };

    class SimpleARFollower {
    public:
        void Reset() noexcept {
            lag_ = 0;
        }

        void SetAttackTime(float ms, float fs) noexcept {
            attack_factor_ = qwqdsp_misc::ExpSmoother::ComputeSmoothFactor(ms, fs, 3);
        }

        void SetReleaseTime(float ms, float fs) noexcept {
            release_factor_ = qwqdsp_misc::ExpSmoother::ComputeSmoothFactor(ms, fs);
        }

        PackFloat<N> Tick(PackFloatCRef<N> x) noexcept {
            auto mask = x  > lag_;
            auto factor = PackOps::Select(mask, attack_factor_, release_factor_);
            lag_ = x + factor * (lag_ - x);
            return lag_;
        }
    private:
        PackFloat<N> lag_{0.0f};
        float attack_factor_{};
        float release_factor_{};
    };

    SimplePeakHold peakhold_;
    SimpleARFollower smoother_;
};
}

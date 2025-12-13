#pragma once
#include <algorithm>
#include "smoother.hpp"

namespace qwqdsp_misc {
class EnevelopeFollower {
public:
    void Reset() noexcept {
        peakhold_.Reset();
        smoother_.Reset();
    }

    float Tick(float x) noexcept {
        x = std::abs(x);
        x = peakhold_.Tick(x);
        return smoother_.Tick(x);
    }

    void SetAttackTime(float ms, float fs) noexcept {
        smoother_.SetAttackTime(ms, fs);
    }

    void SetReleaseTime(float ms, float fs) noexcept {
        smoother_.SetReleaseTime(ms, fs);
    }

    void SetHoldSamples(float ms, float fs) noexcept {
        peakhold_.SetHoldSamples(static_cast<int>(ms * fs / 1000.0f));
    }
private:
    /**
    * @brief a wrong peak hold but it works too.
    */
    class SimplePeakHold {
    public:
        void Reset() noexcept {
            lag_ = 0;
            counter_ = 0;
        }

        float Tick(float x) noexcept {
            --counter_;
            if (x > lag_) {
                lag_ = x;
                counter_ = hold_samples_;
            }
            if (counter_ < 0) {
                lag_ = x;
                counter_ = hold_samples_;
            }
            return lag_;
        }

        void SetHoldSamples(int samples) noexcept {
            hold_samples_ = samples;
            counter_ = std::min(counter_, hold_samples_);
        }
    private:
        float lag_{0.0f};
        int hold_samples_{};
        int counter_{};
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

        float Tick(float x) noexcept {
            auto factor = x > lag_ ? attack_factor_ : release_factor_;
            lag_ = x + factor * (lag_ - x);
            return lag_;
        }
    private:
        float lag_{0.0f};
        float attack_factor_{};
        float release_factor_{};
    };

    SimplePeakHold peakhold_;
    SimpleARFollower smoother_;
};
}

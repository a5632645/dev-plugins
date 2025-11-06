#pragma once
#include <span>
#include "qwqdsp/misc/smoother.hpp"
#include "qwqdsp/filter/int_delay.hpp"
#include "qwqdsp/convert.hpp"

namespace qwqdsp::fx {
/**
 * @brief a simple limiter
 * @ref https://signalsmith-audio.co.uk/writing/2022/limiter/
 */
class SimpleLimiter {
public:
    struct Parameter {
        float release_ms{30.0f};
        float hold_ms{15.0f};
        float lookahead_ms{2.0f};
        float fs{};
        float limit_db{0.0f};
        float makeup_db{0.0f};
    };

    static constexpr float kMaxLookaheadTime = 10.0f;

    void Init(float fs) noexcept {
        lookahead_delay_.Init(fs * kMaxLookaheadTime);
    }

    void Reset() noexcept {
        peakhold_.Reset();
        smoother_.Reset();
        lookahead_delay_.Reset();
    }

    void Update(Parameter const& p) noexcept {
        peakhold_.SetHoldSamples(p.hold_ms * p.fs / 1000.0f);
        smoother_.SetAttackTime(p.lookahead_ms, p.fs / 1000.0f);
        smoother_.SetReleaseTime(p.release_ms, p.fs / 1000.0f);
        lookahead_delay_samples_ = p.lookahead_ms * p.fs / 1000.0f;
        limit_gain_ = qwqdsp::convert::Db2Gain(p.limit_db);
        makeup_gain_ = qwqdsp::convert::Db2Gain(p.makeup_db);
    }

    void Process(std::span<float> block) noexcept {
        for (float& x : block) {
            x *= makeup_gain_;
        }

        float max_abs_peak{};
        for (float x : block) {
            float abs_x = std::abs(x);
            max_abs_peak = std::max(abs_x, max_abs_peak);
        }

        float apply_gain = 1.0f;
        if (max_abs_peak > limit_gain_) {
            apply_gain = limit_gain_ / max_abs_peak;
        }

        for (float& x : block) {
            float g = peakhold_.Tick(-apply_gain);
            g = smoother_.Tick(g);
            g = -g;
            reduce_gain_ = g;

            lookahead_delay_.Push(x);
            float delay_dry = lookahead_delay_.GetAfterPush(lookahead_delay_samples_);

            x = delay_dry * g;
        }
    }

    float GetReduceGain() const noexcept {
        return reduce_gain_;
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
        float lag_{-1.0f};
        int hold_samples_{};
        int counter_{};
    };

    class SimpleARFollower {
    public:
        void Reset() noexcept {
            lag_ = 0;
        }

        void SetAttackTime(float ms, float fs) noexcept {
            attack_factor_ = qwqdsp::misc::ExpSmoother::ComputeSmoothFactor(ms, fs, -3);
        }

        void SetReleaseTime(float ms, float fs) noexcept {
            release_factor_ = qwqdsp::misc::ExpSmoother::ComputeSmoothFactor(ms, fs);
        }

        float Tick(float x) noexcept {
            auto factor = x > lag_ ? attack_factor_ : release_factor_;
            lag_ = x + factor * (lag_ - x);
            return lag_;
        }
    private:
        float lag_{-1.0f};
        float attack_factor_{};
        float release_factor_{};
    };

    SimplePeakHold peakhold_;
    SimpleARFollower smoother_;
    qwqdsp::filter::IntDelay lookahead_delay_;
    size_t lookahead_delay_samples_{};
    float limit_gain_{};
    float reduce_gain_{1.0f};
    float makeup_gain_{};
};
}
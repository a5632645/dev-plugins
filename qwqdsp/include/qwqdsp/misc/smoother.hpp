#pragma once
#include <cmath>

namespace qwqdsp::misc {
class ExpSmoother {
public:
    void Reset() noexcept {
        now_ = target_;
    }

    void SetTarget(float x) noexcept {
        target_ = x;
    }

    void SetTargetImmediately(float x) noexcept {
        now_ = x;
        target_ = x;
    }

    void SetSmoothTime(float ms, float fs) noexcept {
        a_ = ComputeSmoothFactor(ms, fs);
    }

    /**
     * 一阶指数系数,给定单位跃阶将在ms之后到达1/e(63.2%)
     */
    static float ComputeSmoothFactor(float ms, float fs) noexcept {
        float samples = fs * ms / 1000.0f;
        if (samples < 1.0f) {
            return 0.0f;
        }
        else {
            return std::exp(-1.0f / samples);
        }
    }

    /**
     * 一阶指数系数,给定单位跃阶将在ms之后到达close_ratio指定的误差值
     * @param close_ratio 误差值,如0.1% => log10(0.001) => -3
     */
    static float ComputeSmoothFactor(float ms, float fs, float close_ratio) noexcept {
        float samples = fs * ms / 1000.0f;
        if (samples < 1.0f) {
            return 0.0f;
        }
        else {
            return std::pow(10.0f, -close_ratio / samples);
        }
    }

    float Tick() noexcept {
        now_ = target_ + a_ * (now_ - target_);
        return now_;
    }
private:
    float now_{};
    float target_{};
    float a_{};
};

class ConstantTimeSmoother {
public:
    void Reset() noexcept {
        now_ = target_;
        nsamples_ = 0;
    }

    void SetTarget(float x) noexcept {
        target_ = x;
        delta_ = (target_ - now_) / static_cast<float>(total_samples_);
        nsamples_ = total_samples_;
    }

    void SetSmoothTime(float ms, float fs) noexcept {
        total_samples_ = static_cast<int>(fs * ms / 1000.0f);
        total_samples_ = total_samples_ < 1 ? 1 : total_samples_;
        delta_ = (target_ - now_) / static_cast<float>(total_samples_);
    }

    bool IsEnd() const noexcept {
        return nsamples_ == 0;
    }

    float Tick() noexcept {
        if (IsEnd()) return target_;
        now_ += delta_;
        --nsamples_;
        return now_;
    }
private:
    float target_{};
    float now_{};
    float delta_{};
    int nsamples_{};
    int total_samples_{};
};

class ContantValueSmoother {
public:
    void Reset() noexcept {
        now_ = target_;
        end_ = true;
    }

    void SetTarget(float x) noexcept {
        target_ = x;
        if (target_ < now_) {
            delta_ = -max_delta_;
        }
        else {
            delta_ = max_delta_;
        }
        end_ = false;
    }

    void SetMaxValueMove(float x) noexcept {
        max_delta_ = std::abs(x);
    }

    bool IsEnd() const noexcept {
        return end_;
    }

    float Tick() noexcept {
        if (IsEnd()) return target_;
        now_ += target_;
        if (std::abs(target_ - now_) < delta_) end_ = true;
        return now_;
    }
private:
    float now_{};
    float target_{};
    float max_delta_{};
    float delta_{};
    bool end_{};
};

class ImSmoother {
public:
    void Reset(float val) noexcept {
        current_ = val;
    }

    void Begin(float target, float num_samples) noexcept {
        inc_ = (target - current_) / num_samples;
    }

    float Tick() noexcept {
        current_ += inc_;
        return current_;
    }
private:
    float current_{};
    float inc_{};
};
}
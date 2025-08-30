#pragma once
#include <cmath>

namespace qwqdsp {
class ExpSmoother {
public:
    void SetTarget(float x) {
        target_ = x;
    }

    void SetSmoothTime(float ms, float fs) {
        a_ = std::exp(-1.0f / (fs * ms / 1000.0f));
    }

    float Tick() {
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
    void SetTarget(float x) {
        target_ = x;
        delta_ = (target_ - now_) / total_samples_;
        nsamples_ = total_samples_;
    }

    void SetSmoothTime(float ms, float fs) {
        total_samples_ = fs * ms / 1000.0f;
        total_samples_ = total_samples_ < 1 ? 1 : total_samples_;
        delta_ = (target_ - now_) / total_samples_;
    }

    bool IsEnd() const {
        return nsamples_ == 0;
    }

    float Tick() {
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
    void SetTarget(float x) {
        target_ = x;
        if (target_ < now_) {
            delta_ = -max_delta_;
        }
        else {
            delta_ = max_delta_;
        }
        end_ = false;
    }

    void SetMaxValueMove(float x) {
        max_delta_ = std::abs(x);
    }

    bool IsEnd() const {
        return end_;
    }

    float Tick() {
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
}
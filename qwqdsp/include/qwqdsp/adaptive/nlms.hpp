#pragma once
#include "lag_buffer.hpp"

namespace qwqdsp::adaptive {
template<size_t kSize, float kStep = 0.1f>
class NLMS {
public:
    void Reset() noexcept {
        w_.fill(float{});
    }

    /**
     * @return 预测
     */
    float Tick(float x, float target) noexcept {
        x_.Push(x);

        float pred{};
        float sum{};
        for (size_t i = 0; i < kSize; ++i) {
            pred += w_[i] * x_[i];
            sum += x_[i] * x_[i];
        }

        float const e = target - pred;
        for (size_t i = 0; i < kSize; ++i) {
            w_[i] += kStep * e * x_[i] / (sum + 1e-3f);
        }

        return pred;
    }
private:
    LagBufferStatic<kSize> x_;
    std::array<float, kSize> w_{};
};
}

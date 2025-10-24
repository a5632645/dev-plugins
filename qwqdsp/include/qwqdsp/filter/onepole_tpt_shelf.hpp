#pragma once
#include <cmath>

namespace qwqdsp::filter {
/**
 * @brief let G(s) = 1/(1+s)
 *        M = 10^(db/40)
 *        Highshelf = G(s/M)/G(Ms)
 *        Tiltshelf = Highshelf/M
 *        Low shelf = Tiltshelf/M
 */
class OnepoleTPTShelf {
public:
    void Reset() noexcept {
        lag_ = 0;
    }

    /**
     * @param w [0, pi]
     * @param gain 10^(db/40)
     *        low shelf -> H(0)=-db*2 H(pi)= 1
     *       tilt shelf -> H(0)= -db  H(pi)= db
     *       high shelf -> H(0)=  1   H(pi)=2*db
     * @note 如果是lowshelf, 提供-db才会是增强db
     */
    void Set(float w, float gain) noexcept {
        m_ = gain;
        invm_ = 1 / gain;
        float const k = std::tan(w / 2) * m_;
        g_ = k / (1 + k);
    }

    float TickLowshelf(float x) noexcept {
        float lp = TickLowpass(x);
        return invm_ * invm_ * lp + (x - lp);
    }

    float TickHighshelf(float x) noexcept {
        float lp = TickLowpass(x);
        return lp + m_ * m_ * (x - lp);
    }

    float TickTiltshelf(float x) noexcept {
        float lp = TickLowpass(x);
        return lp * invm_ + m_ * (x - lp);
    }

    float TickLowpass(float x) noexcept {
        float const delta = g_ * (x - lag_);
        lag_ += delta;
        float const y = lag_;
        lag_ += delta;
        return y;
    }
private:
    float g_{};
    float m_{};
    float invm_{};
    float lag_{};
};
}

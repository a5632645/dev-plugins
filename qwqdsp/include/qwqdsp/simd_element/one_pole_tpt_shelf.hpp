#pragma once
#include "simd_pack.hpp"

namespace qwqdsp_simd_element {
/**
 * @brief let G(s) = 1/(1+s)
 *        M = 10^(db/40)
 *        Highshelf = G(s/M)/G(Ms)
 *        Tiltshelf = Highshelf/M
 *        Low shelf = Tiltshelf/M
 */
template<size_t N>
class OnepoleTPTShelf {
public:
    void Reset() noexcept {
        lag_.Broadcast(0);
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
        m_.Broadcast(gain);
        invm_.Broadcast(1 / gain);
        float const k = std::tan(w / 2) * gain;
        g_.Broadcast(k / (1 + k));
    }

    PackFloat<N> TickLowshelf(PackFloatCRef<N> x) noexcept {
        float lp = TickLowpass(x);
        return invm_ * invm_ * lp + (x - lp);
    }

    PackFloat<N> TickHighshelf(PackFloatCRef<N> x) noexcept {
        float lp = TickLowpass(x);
        return lp + m_ * m_ * (x - lp);
    }

    PackFloat<N> TickTiltshelf(PackFloatCRef<N> x) noexcept {
        auto lp = TickLowpass(x);
        return lp * invm_ + m_ * (x - lp);
    }

    PackFloat<N> TickLowpass(PackFloatCRef<N> x) noexcept {
        auto delta = g_ * (x - lag_);
        lag_ += delta;
        auto y = lag_;
        lag_ += delta;
        return y;
    }
private:
    PackFloat<N> g_{};
    PackFloat<N> m_{};
    PackFloat<N> invm_{};
    PackFloat<N> lag_{};
};
}

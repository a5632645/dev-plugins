#pragma once
#include "../simd.hpp"

namespace pluginshared::dsp {

template <class SimdT>
class StereoIIRHilbertCpx2 {
public:
    StereoIIRHilbertCpx2() noexcept {
        auto* f = filters_;
        for (int i = 0; i < kState; i += simd::LaneSize<SimdT>) {
            if constexpr (std::is_same_v<SimdT, simd::Float128>) {
                f->a_ = simd::Loadu128(a + i);
                f->b_ = simd::Loadu128(b + i);
                f->c_ = simd::Loadu128(c + i);
                f->d_ = simd::Loadu128(d + i);
            }
            else {
                f->a_ = simd::Loadu256(a + i);
                f->b_ = simd::Loadu256(b + i);
                f->c_ = simd::Loadu256(c + i);
                f->d_ = simd::Loadu256(d + i);
            }
            ++f;
        }
    }

    void Reset() noexcept {
        for (auto& f : filters_) {
            f.Reset();
        }
    }

    /**
     * @param x => [L_re, L_im, R_re, R_im]
     * @return  => [L_re, L_im, R_re, R_im]
     */
    simd::Float128 Tick(simd::Float128 x) noexcept {
        auto y = x * direct_;

        SimdT lre{};
        SimdT lim{};
        SimdT rre{};
        SimdT rim{};
        for (auto& f : filters_) {
            f.Tick(x, lre, lim, rre, rim);
        }

        y[0] += simd::ReduceAdd(lre);
        y[1] += simd::ReduceAdd(lim);
        y[2] += simd::ReduceAdd(rre);
        y[3] += simd::ReduceAdd(rim);
        return y * 2;
    }
private:
    template <class SimdT2>
    class Filter {
    public:
        void Reset() noexcept {
            sre1_l_ = 0;
            sre2_l_ = 0;
            sim1_l_ = 0;
            sim2_l_ = 0;

            sre1_r_ = 0;
            sre2_r_ = 0;
            sim1_r_ = 0;
            sim2_r_ = 0;
        }

        void Tick(simd::Float128 x, SimdT2& lre, SimdT2& lim, SimdT2& rre, SimdT2& rim) noexcept {
            float xre = x[0];
            float xim = x[1];
            SimdT2 yre = xre * a_ + sre1_l_;
            SimdT2 yim = xim * a_ + sim1_l_;
            sre1_l_ = b_ * xim - c_ * yim + sre2_l_;
            sim1_l_ = -b_ * xre + yre * c_ + sim2_l_;
            sre2_l_ = d_ * yre;
            sim2_l_ = d_ * yim;
            lre += yre;
            lim += yim;

            xre = x[2];
            xim = x[3];
            yre = xre * a_ + sre1_r_;
            yim = xim * a_ + sim1_r_;
            sre1_r_ = b_ * xim - c_ * yim + sre2_r_;
            sim1_r_ = -b_ * xre + yre * c_ + sim2_r_;
            sre2_r_ = d_ * yre;
            sim2_r_ = d_ * yim;
            rre += yre;
            rim += yim;
        }

        SimdT2 a_{};
        SimdT2 b_{};
        SimdT2 c_{};
        SimdT2 d_{};
    private:
        SimdT2 sre1_l_{};
        SimdT2 sre2_l_{};
        SimdT2 sim1_l_{};
        SimdT2 sim2_l_{};

        SimdT2 sre1_r_{};
        SimdT2 sre2_r_{};
        SimdT2 sim1_r_{};
        SimdT2 sim2_r_{};
    };

    static constexpr float a[16]{
        1.5117691950e-01f,  -4.0029448985e-01f, 4.9774027954e-01f,  -4.4633141833e-01f,
        3.2473980264e-01f,  -1.9996267924e-01f, 1.0336160540e-01f,  -4.0935386859e-02f,
        6.8327132448e-03f,  7.9749493092e-03f,  -1.1561746436e-02f, 9.7259201265e-03f,
        -6.0695498296e-03f, 2.6117239910e-03f,  -4.4278853955e-04f, -1.1149996062e-04f,
    };

    static constexpr float b[16]{
        -1.6150960039e-01f, 3.1285144583e-01f,  -2.3897509472e-01f, 9.8686295334e-02f,
        1.0576186212e-02f,  -6.5895779752e-02f, 7.8355485382e-02f,  -6.6911037420e-02f,
        4.7011218501e-02f,  -2.7827970196e-02f, 1.3290298175e-02f,  -4.0972773302e-03f,
        -5.6086795657e-04f, 1.9432727706e-03f,  -1.3746116533e-03f, 3.8518024932e-04f,
    };

    static constexpr float c[16]{
        1.3052689240e+00f, 1.1135166236e+00f, 8.3791368666e-01f, 5.8031118445e-01f,
        3.8228975485e-01f, 2.4442612941e-01f, 1.5330819938e-01f, 9.4761364871e-02f,
        5.7737148988e-02f, 3.4545671410e-02f, 2.0113644317e-02f, 1.1189489279e-02f,
        5.7275727638e-03f, 2.4630086794e-03f, 6.3520964134e-04f, -1.8305928139e-04f,
    };

    static constexpr float d[16]{
        4.5547132049e-01f, 5.3524281193e-01f, 6.4989752517e-01f, 7.5706439789e-01f,
        8.3944569600e-01f, 8.9680198411e-01f, 9.3471366069e-01f, 9.5907883314e-01f,
        9.7449591774e-01f, 9.8416742751e-01f, 9.9020946329e-01f, 9.9398370257e-01f,
        9.9635568114e-01f, 9.9787510426e-01f, 9.9889639359e-01f, 9.9965877371e-01f,
    };

    static constexpr float direct_ = 0.0016725282372669745f;
    static constexpr int kState = 16;
    Filter<SimdT> filters_[kState / simd::LaneSize<SimdT>];
};

} // namespace pluginshared::dsp

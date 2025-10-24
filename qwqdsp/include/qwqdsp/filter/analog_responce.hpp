#pragma once
#include <complex>

namespace qwqdsp::filter {
class AnalogResponce {
public:
    /**
     * @param w rad/s
     * @param wc 截止频率
     */
    std::complex<float> LowpassOnepole(float w, float wc) noexcept {
        std::complex<float> s{0.0f, w/wc};
        return 1.0f / (1.0f + s);
    }

    /**
     * @param w rad/s
     * @param wc 截止频率
     */
    std::complex<float> HighpassOnepole(float w, float wc) noexcept {
        std::complex<float> s{0.0f, w/wc};
        return s / (1.0f + s);
    }

    /**
     * @brief DB=0dB wc=db/2 inf=db
     * @param w rad/s
     * @param wc 截止频率
     * @param sqrt_A 10^db/40
     */
    std::complex<float> HighshelfOnepole(float w, float wc, float sqrt_A) noexcept {
        std::complex<float> s{0.0f, w/wc};
        return (1.0f+sqrt_A*s)/(1.0f+s/sqrt_A);
    }

    /**
     * @brief DC=-db/2 wc=0 inf=db/2
     * @param w rad/s
     * @param wc 截止频率
     * @param sqrt_A 10^db/40
     */
    std::complex<float> TiltshelfOnepole(float w, float wc, float sqrt_A) noexcept {
        std::complex<float> s{0.0f, w/wc};
        return (1.0f+sqrt_A*s)/(s+sqrt_A);
    }

    /**
     * @brief DC=db wc=db/2 inf=0dB
     * @param w rad/s
     * @param wc 截止频率
     * @param sqrt_A 10^db/40
     */
    std::complex<float> LowshelfOnepole(float w, float wc, float sqrt_A) noexcept {
        std::complex<float> s{0.0f, w/wc};
        return (sqrt_A*sqrt_A+s*sqrt_A)/(s*sqrt_A+1.0f);
    }

    /**
     * @param w rad/s
     * @param wc 截止频率
     */
    std::complex<float> AllpassOnepole(float w, float wc) noexcept {
        std::complex<float> s{0.0f, w/wc};
        return (1.0f-s)/(1.0f+s);
    }

    /**
     * @param w rad/s
     * @param wc 截止频率
     * @param Q 品质因子
     */
    std::complex<float> Lowpass(float w, float wc, float Q) noexcept {
        std::complex<float> s{0.0f, w/wc};
        return 1.0f/(s*s+s/Q+1.0f);
    }

    /**
     * @param w rad/s
     * @param wc 截止频率
     * @param Q 品质因子
     */
    std::complex<float> Highpass(float w, float wc, float Q) noexcept {
        std::complex<float> s{0.0f, w/wc};
        return s*s/(s*s+s/Q+1.0f);
    }

    /**
     * @param w rad/s
     * @param wc 截止频率
     * @param Q 品质因子
     */
    std::complex<float> Bandpass(float w, float wc, float Q) noexcept {
        std::complex<float> s{0.0f, w/wc};
        return s/(s*s+s/Q+1.0f);
    }

    /**
     * @brief 截止频率始终为1的带通滤波器
     * @param w rad/s
     * @param wc 截止频率
     * @param Q 品质因子
     */
    std::complex<float> NormBandpass(float w, float wc, float Q) noexcept {
        std::complex<float> s{0.0f, w/wc};
        return (s/Q)/(s*s+s/Q+1.0f);
    }

    /**
     * @param w rad/s
     * @param wc 截止频率
     * @param Q 品质因子
     */
    std::complex<float> Notch(float w, float wc, float Q) noexcept {
        std::complex<float> s{0.0f, w/wc};
        return (s*s+1.0f)/(s*s+s/Q+1.0f);
    }

    /**
     * @param w rad/s
     * @param wc 截止频率
     * @param Q 品质因子
     */
    std::complex<float> Allpass(float w, float wc, float Q) noexcept {
        std::complex<float> s{0.0f, w/wc};
        auto s2 = s*s;
        return (s2-s/Q+1.0f)/(s2+s/Q+1.0f);
    }

    /**
     * @param w rad/s
     * @param wc 截止频率
     * @param Q 品质因子
     * @param A 10^db/40
     */
    std::complex<float> Peaking(float w, float wc, float Q, float A) noexcept {
        std::complex<float> s{0.0f, w/wc};
        auto s2 = s*s;
        return (s2+s*A/Q+1.0f)/(s2+s/A/Q+1.0f);
    }

    /**
     * @brief DC=db, wc=db/2, inf=0dB
     * @param w rad/s
     * @param wc 截止频率
     * @param Q 品质因子
     * @param A 10^db/80
     */
    std::complex<float> Lowshelf(float w, float wc, float Q, float sqrt_A) noexcept {
        std::complex<float> s{0.0f, w/wc};
        auto s2 = s*s;
        auto A = sqrt_A*sqrt_A;
        return A*(s2+sqrt_A/Q*s+A)/(A*s2+sqrt_A/Q*s+1.0f);
    }

    /**
     * @brief DC=0dB, wc=db/2, inf=db
     * @param w rad/s
     * @param wc 截止频率
     * @param Q 品质因子
     * @param A 10^db/80
     */
    std::complex<float> Highshelf(float w, float wc, float Q, float sqrt_A) noexcept {
        std::complex<float> s{0.0f, w/wc};
        auto s2 = s*s;
        auto A = sqrt_A*sqrt_A;
        return A*(A*s2+sqrt_A/Q*s+1.0f)/(s2+sqrt_A/Q*s+A);
    }

    /**
     * @brief DC=-db/2, wc=0, inf=db/2
     * @param w rad/s
     * @param wc 截止频率
     * @param Q 品质因子
     * @param A 10^db/80
     */
    std::complex<float> Tiltshelf(float w, float wc, float Q, float sqrt_A) noexcept {
        std::complex<float> s{0.0f, w/wc};
        auto s2 = s*s;
        auto A = sqrt_A*sqrt_A;
        return (A*s2+sqrt_A/Q*s+1.0f)/(s2+sqrt_A/Q*s+A);
    }
};
}
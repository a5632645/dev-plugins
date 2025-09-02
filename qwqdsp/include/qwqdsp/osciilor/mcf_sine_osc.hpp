#pragma once
#include <cmath>
#include <numbers>
#include <complex>

namespace qwqdsp::oscillor {
/**
 * @brief 修正耦合正弦振荡器，又称Magic circle，等振幅，在低频下接近正交输出
 * 这个实现使用了后向差分来求解余弦，在较小的频率更改下相位可能不正确
 * @ref https://quod.lib.umich.edu/cgi/p/pod/dod-idx/sine-generation-algorithm-for-vlsi-applications.pdf?c=icmc;idno=bbp2372.1985.028;format=pdf
 */
class MCFSineOsc {
public:
    void Reset(float f, float fs, float p) noexcept {
        auto omega = f / fs * std::numbers::pi_v<float> * 2.0f;
        float phi = (std::numbers::pi_v<float> - omega) / 2.0f;
        sin0_ = std::sin(p);
        sin1_ = std::sin(p - phi);
        coeff_ = 2.0f * std::sin(omega / 2.0f);
    }

    [[nodiscard("this method will update the oscillor state")]]
    float SetFreq(float f, float fs) noexcept {
        auto omega = f / fs * std::numbers::pi_v<float> * 2.0f;
        auto ret = sin0_;
        sin0_ -= coeff_ * sin1_;
        sin1_ += coeff_ * sin0_;
        if (sin0_ > ret) {
            float predCos = LimitCosConvert(sin0_);
            coeff_ = 2.0f * std::sin(omega / 2.0f);
            sin1_ = sin0_ * std::sin(omega / 2.0f) - predCos * std::cos(omega / 2.0f);
        }
        else {
            float predCos = -LimitCosConvert(sin0_);
            coeff_ = 2.0f * std::sin(omega / 2.0f);
            sin1_ = sin0_ * std::sin(omega / 2.0f) - predCos * std::cos(omega / 2.0f);
        }
        return sin0_;
    }

    float Tick() noexcept {
        sin0_ -= sin1_ * coeff_;
        sin1_ += sin0_ * coeff_;
        return sin0_;
    }
private:
    static float LimitCosConvert(float sin) noexcept {
        auto e = 1.0f - sin * sin;
        if (e < 0.0f) {
            return 0.0f;
        }
        else if (e > 1.0f) {
            return 1.0f;
        }
        else {
            return std::sqrt(e);
        }
    }

    float sin0_{};
    float sin1_{};
    float coeff_{};
};

/**
 * @brief 完整的正交MCF振荡器
 * 允许在低量化精度下工作，衰减慢
 * @ref https://quod.lib.umich.edu/cgi/p/pod/dod-idx/sine-generation-algorithm-for-vlsi-applications.pdf?c=icmc;idno=bbp2372.1985.028;format=pdf
 */
class FullMCFSineOsc {
public:
    void Reset(float f, float fs, float phase) {
        auto omega = f / fs * std::numbers::pi_v<float> * 2.0f;
        auto phi = (std::numbers::pi_v<float> - omega) / 2.0f;
        sin0_ = std::sin(phase);
        sin1_ = std::sin(phase - phi);
        cos0_ = std::cos(phase);
        cos1_ = std::cos(phase - phi);
        coeff_ = 2.0f * std::sin(omega / 2.0f);
    }

    float Tick() {
        sin0_ -= sin1_ * coeff_;
        sin1_ += sin0_ * coeff_;
        cos0_ -= cos1_ * coeff_;
        cos1_ += cos0_ * coeff_;
        return sin0_;
    }

    float Cosine() const {
        return cos0_;
    }

    void SetFreq(float f, float fs) {
        auto omega = f / fs * std::numbers::pi_v<float> * 2.0f;
        coeff_ = 2.0f * std::sin(omega / 2.0f);
        auto new_phi = (std::numbers::pi_v<float> - omega) / 2.0f;
        auto cosphi = std::cos(new_phi);
        auto sinphi = std::sin(new_phi);
        sin1_ = sin0_ * cosphi - cos0_ * sinphi;
        cos1_ = cos0_ * cosphi + sin0_ * sinphi;
    }

    std::complex<float> GetCpx() const {
        return {cos0_, sin0_};
    }
private:
    float sin0_{};
    float sin1_{};
    float cos0_{};
    float cos1_{};
    float coeff_{};
};
}
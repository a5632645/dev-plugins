#pragma once
#include <array>
#include <qwqdsp/filter/iir_design.hpp>
#include <qwqdsp/filter/iir_design_extra.hpp>
#include <qwqdsp/simd_element/simd_pack.hpp>

#include "../global.hpp"

namespace green_vocoder::dsp {
class TwoBandSVF {
public:
    /**
     * @brief
     *
     * @tparam kOnlyPole
     * true意味着你是通过Q和w计算出来的带通滤波器，false为可能携带零点的传统滤波器设计方法(qwqdsp_filter::IIRDesign)
     * @param v0_l
     * @param v0_r
     */
    template <bool kOnlyPole>
    void Tick(qwqdsp_simd_element::PackFloat<4>& v0_l, qwqdsp_simd_element::PackFloat<4>& v0_r) noexcept {
        if constexpr (!kOnlyPole) {
            qwqdsp_simd_element::PackFloat<4> f1_y_l;
            qwqdsp_simd_element::PackFloat<4> f1_y_r;
            {
                auto hp_in_l = v0_l * f1_.hp_mix_;
                auto lp_in_l = v0_l * f1_.lp_mix_;
                auto y_l = (hp_in_l + f1_.g_ * f1_.g_ * lp_in_l + f1_.g_ * f1_.s1_l_ + f1_.s2_l_) * f1_.d_;
                f1_y_l = y_l;
                auto w1_l = lp_in_l - y_l;
                auto w2_l = f1_.g_ * w1_l + f1_.s1_l_;
                f1_.s1_l_ = w2_l + f1_.g_ * w1_l;
                w2_l = w2_l - f1_.r2_ * y_l;
                auto w3_l = f1_.g_ * w2_l + f1_.s2_l_;
                f1_.s2_l_ = w3_l + f1_.g_ * w2_l;

                auto hp_in_r = v0_r * f1_.hp_mix_;
                auto lp_in_r = v0_r * f1_.lp_mix_;
                auto y_r = (hp_in_r + f1_.g_ * f1_.g_ * lp_in_r + f1_.g_ * f1_.s1_r_ + f1_.s2_r_) * f1_.d_;
                f1_y_r = y_r;
                auto w1_r = lp_in_r - y_r;
                auto w2_r = f1_.g_ * w1_r + f1_.s1_r_;
                f1_.s1_r_ = w2_r + f1_.g_ * w1_r;
                w2_r = w2_r - f1_.r2_ * y_r;
                auto w3_r = f1_.g_ * w2_r + f1_.s2_r_;
                f1_.s2_r_ = w3_r + f1_.g_ * w2_r;
            }

            qwqdsp_simd_element::PackFloat<4> f2_y_l;
            qwqdsp_simd_element::PackFloat<4> f2_y_r;
            {
                auto hp_in_l = f1_y_l * f2_.hp_mix_;
                auto lp_in_l = f1_y_l * f2_.lp_mix_;
                auto y_l = (hp_in_l + f2_.g_ * f2_.g_ * lp_in_l + f2_.g_ * f2_.s1_l_ + f2_.s2_l_) * f2_.d_;
                f2_y_l = y_l;
                auto w1_l = lp_in_l - f2_y_l;
                auto w2_l = f2_.g_ * w1_l + f2_.s1_l_;
                f2_.s1_l_ = w2_l + f2_.g_ * w1_l;
                w2_l = w2_l - f2_.r2_ * f2_y_l;
                auto w3_l = f2_.g_ * w2_l + f2_.s2_l_;
                f2_.s2_l_ = w3_l + f2_.g_ * w2_l;

                auto hp_in_r = f1_y_r * f2_.hp_mix_;
                auto lp_in_r = f1_y_r * f2_.lp_mix_;
                auto y_r = (hp_in_r + f2_.g_ * f2_.g_ * lp_in_r + f2_.g_ * f2_.s1_r_ + f2_.s2_r_) * f2_.d_;
                f2_y_r = y_r;
                auto w1_r = lp_in_r - f2_y_r;
                auto w2_r = f2_.g_ * w1_r + f2_.s1_r_;
                f2_.s1_r_ = w2_r + f2_.g_ * w1_r;
                w2_r = w2_r - f2_.r2_ * f2_y_r;
                auto w3_r = f2_.g_ * w2_r + f2_.s2_r_;
                f2_.s2_r_ = w3_r + f2_.g_ * w2_r;
                v0_l = f2_y_l;
                v0_r = f2_y_r;
            }
        }
        else {
            // normalized bandpass
            auto bp_l = f1_.d_ * (f1_.g_ * (v0_l * f1_.r2_ - f1_.s2_l_) + f1_.s1_l_);
            auto bp_r = f1_.d_ * (f1_.g_ * (v0_r * f1_.r2_ - f1_.s2_r_) + f1_.s1_r_);
            auto bp2_l = bp_l + bp_l;
            auto bp2_r = bp_r + bp_r;
            f1_.s1_l_ = bp2_l - f1_.s1_l_;
            f1_.s1_r_ = bp2_r - f1_.s1_r_;
            auto v22_l = f1_.g_ * bp2_l;
            auto v22_r = f1_.g_ * bp2_r;
            f1_.s2_l_ += v22_l;
            f1_.s2_r_ += v22_r;
            auto y0_l = bp_l;
            auto y0_r = bp_r;

            bp_l = f2_.d_ * (f2_.g_ * (y0_l * f2_.r2_ - f2_.s2_l_) + f2_.s1_l_);
            bp_r = f2_.d_ * (f2_.g_ * (y0_r * f2_.r2_ - f2_.s2_r_) + f2_.s1_r_);
            bp2_l = bp_l + bp_l;
            bp2_r = bp_r + bp_r;
            f2_.s1_l_ = bp2_l - f2_.s1_l_;
            f2_.s1_r_ = bp2_r - f2_.s1_r_;
            v22_l = f2_.g_ * bp2_l;
            v22_r = f2_.g_ * bp2_r;
            f2_.s2_l_ += v22_l;
            f2_.s2_r_ += v22_r;
            v0_l = bp_l;
            v0_r = bp_r;
        }
    }

    void MakeBandpass(qwqdsp_simd_element::PackFloatCRef<4> analog_w, qwqdsp_simd_element::PackFloatCRef<4> Q,
                      qwqdsp_simd_element::PackFloatCRef<4> analog_w2,
                      qwqdsp_simd_element::PackFloatCRef<4> Q2) noexcept {
        f1_.g_ = analog_w;
        f1_.r2_ = 1.0f / Q;
        f1_.d_ = 1.0f / (1.0f + f1_.r2_ * f1_.g_ + f1_.g_ * f1_.g_);

        f2_.g_ = analog_w2;
        f2_.r2_ = 1.0f / Q2;
        f2_.d_ = 1.0f / (1.0f + f2_.r2_ * f2_.g_ + f2_.g_ * f2_.g_);
    }

    void SetAnalogPoleZero(std::array<qwqdsp_filter::IIRDesign::ZPK, 4> const& zpk_buffer,
                           std::array<qwqdsp_filter::IIRDesign::ZPK, 4> const& zpk_buffer2) noexcept {
        // poles
        qwqdsp_simd_element::PackFloat<4> p_re{
            static_cast<float>(zpk_buffer[0].p.real()), static_cast<float>(zpk_buffer[1].p.real()),
            static_cast<float>(zpk_buffer[2].p.real()), static_cast<float>(zpk_buffer[3].p.real())};
        qwqdsp_simd_element::PackFloat<4> p_im{
            static_cast<float>(zpk_buffer[0].p.imag()), static_cast<float>(zpk_buffer[1].p.imag()),
            static_cast<float>(zpk_buffer[2].p.imag()), static_cast<float>(zpk_buffer[3].p.imag())};
        auto analog_w = qwqdsp_simd_element::PackOps::Sqrt(p_re * p_re + p_im * p_im);
        auto Q = analog_w / (-2.0f * p_re);
        Q = qwqdsp_simd_element::PackOps::Abs(Q);
        f1_.g_ = analog_w;
        f1_.r2_ = 1.0f / Q;
        f1_.d_ = 1.0f / (1.0f + f1_.r2_ * f1_.g_ + f1_.g_ * f1_.g_);

        p_re = {static_cast<float>(zpk_buffer2[0].p.real()), static_cast<float>(zpk_buffer2[1].p.real()),
                static_cast<float>(zpk_buffer2[2].p.real()), static_cast<float>(zpk_buffer2[3].p.real())};
        p_im = {static_cast<float>(zpk_buffer2[0].p.imag()), static_cast<float>(zpk_buffer2[1].p.imag()),
                static_cast<float>(zpk_buffer2[2].p.imag()), static_cast<float>(zpk_buffer2[3].p.imag())};
        analog_w = qwqdsp_simd_element::PackOps::Sqrt(p_re * p_re + p_im * p_im);
        Q = analog_w / (-2.0f * p_re);
        Q = qwqdsp_simd_element::PackOps::Abs(Q);
        f2_.g_ = analog_w;
        f2_.r2_ = 1.0f / Q;
        f2_.d_ = 1.0f / (1.0f + f2_.r2_ * f2_.g_ + f2_.g_ * f2_.g_);

        // zeros
        // other channels always match this condition
        if (zpk_buffer[0].z) {
            qwqdsp_simd_element::PackFloat<4> z_re{
                static_cast<float>((*zpk_buffer[0].z).real()), static_cast<float>((*zpk_buffer[1].z).real()),
                static_cast<float>((*zpk_buffer[2].z).real()), static_cast<float>((*zpk_buffer[3].z).real())};
            qwqdsp_simd_element::PackFloat<4> z_im{
                static_cast<float>((*zpk_buffer[0].z).imag()), static_cast<float>((*zpk_buffer[1].z).imag()),
                static_cast<float>((*zpk_buffer[2].z).imag()), static_cast<float>((*zpk_buffer[3].z).imag())};
            qwqdsp_simd_element::PackFloat<4> k{
                static_cast<float>(zpk_buffer[0].k), static_cast<float>(zpk_buffer[1].k),
                static_cast<float>(zpk_buffer[2].k), static_cast<float>(zpk_buffer[3].k)};
            f1_.lp_mix_ = k * (z_re * z_re + z_im * z_im) / (f1_.g_ * f1_.g_);
            f1_.hp_mix_ = k;
        }
        else {
            qwqdsp_simd_element::PackFloat<4> k{
                static_cast<float>(zpk_buffer[0].k), static_cast<float>(zpk_buffer[1].k),
                static_cast<float>(zpk_buffer[2].k), static_cast<float>(zpk_buffer[3].k)};
            f1_.lp_mix_ = k / (f1_.g_ * f1_.g_);
            f1_.hp_mix_.Broadcast(0);
        }

        if (zpk_buffer2[0].z) {
            qwqdsp_simd_element::PackFloat<4> z_re{
                static_cast<float>((*zpk_buffer2[0].z).real()), static_cast<float>((*zpk_buffer2[1].z).real()),
                static_cast<float>((*zpk_buffer2[2].z).real()), static_cast<float>((*zpk_buffer2[3].z).real())};
            qwqdsp_simd_element::PackFloat<4> z_im{
                static_cast<float>((*zpk_buffer2[0].z).imag()), static_cast<float>((*zpk_buffer2[1].z).imag()),
                static_cast<float>((*zpk_buffer2[2].z).imag()), static_cast<float>((*zpk_buffer2[3].z).imag())};
            qwqdsp_simd_element::PackFloat<4> k{
                static_cast<float>(zpk_buffer2[0].k), static_cast<float>(zpk_buffer2[1].k),
                static_cast<float>(zpk_buffer2[2].k), static_cast<float>(zpk_buffer2[3].k)};
            f2_.lp_mix_ = k * (z_re * z_re + z_im * z_im) / (f2_.g_ * f2_.g_);
            f2_.hp_mix_ = k;
        }
        else {
            qwqdsp_simd_element::PackFloat<4> k{
                static_cast<float>(zpk_buffer2[0].k), static_cast<float>(zpk_buffer2[1].k),
                static_cast<float>(zpk_buffer2[2].k), static_cast<float>(zpk_buffer2[3].k)};
            f2_.lp_mix_ = k / (f2_.g_ * f2_.g_);
            f2_.hp_mix_.Broadcast(0);
        }
    }

    void Reset() noexcept {
        f1_.s1_l_.Broadcast(0);
        f1_.s1_r_.Broadcast(0);
        f1_.s2_r_.Broadcast(0);
        f1_.s2_l_.Broadcast(0);
        f2_.s1_l_.Broadcast(0);
        f2_.s1_r_.Broadcast(0);
        f2_.s2_r_.Broadcast(0);
        f2_.s2_l_.Broadcast(0);
    }
private:
    struct OneData {
        qwqdsp_simd_element::PackFloat<4> s2_l_{};
        qwqdsp_simd_element::PackFloat<4> s2_r_{};
        qwqdsp_simd_element::PackFloat<4> s1_l_{};
        qwqdsp_simd_element::PackFloat<4> s1_r_{};
        qwqdsp_simd_element::PackFloat<4> r2_{};
        qwqdsp_simd_element::PackFloat<4> g_{};
        qwqdsp_simd_element::PackFloat<4> d_{};
        qwqdsp_simd_element::PackFloat<4> hp_mix_{};
        qwqdsp_simd_element::PackFloat<4> lp_mix_{};
    };
    OneData f1_;
    OneData f2_;
};

struct CascadeBPSVF {
    template <size_t kNumFilters, bool kOnlyPole>
    void Tick(qwqdsp_simd_element::PackFloat<4>& l, qwqdsp_simd_element::PackFloat<4>& r) noexcept {
        for (int i = 0; i < static_cast<int>(kNumFilters / 2); ++i) {
            svf_[static_cast<size_t>(i)].Tick<kOnlyPole>(l, r);
        }
    }

    void Reset() noexcept {
        for (auto& f : svf_) {
            f.Reset();
        }
    }

    std::array<TwoBandSVF, 3> svf_;
};

enum eChannelVocoderMap {
    eChannelVocoderMap_Linear = 0,
    eChannelVocoderMap_Mel,
    eChannelVocoderMap_Log,
    eChannelVocoderMap_NumEnums
};

class ChannelVocoder {
public:
    enum class FilterBankMode {
        Bandpass12,
        StackButterworth24,
        StackButterworth36,
        FlatButterworth24,
        FlatButterworth36,
        Chebyshev24,
        Chebyshev36,
        Elliptic24,
        Elliptic36,
    };

    struct Params {
        int num_bands{20};
        float freq_begin{40.0f};
        float freq_end{12000.0f};
        float attack{10.0f};
        float release{150.0f};
        float modulator_scale{1.0f};
        float carry_scale{1.0f};
        eChannelVocoderMap map{eChannelVocoderMap_Mel};
        FilterBankMode filter_bank_mode{FilterBankMode::StackButterworth24};
        float gate{-100.0f};
        float formant_shift{0.0f};
        float ripple{1.0f};
    };

    void Init(float sample_rate, int block_size);
    void ProcessBlock(qwqdsp_simd_element::PackFloat<2>* main, qwqdsp_simd_element::PackFloat<2>* side,
                      int num_samples);

    void SetParam(const Params& p);

    int GetNumBins() const {
        return num_bans_;
    }
    qwqdsp_simd_element::PackFloat<2> GetBinPeak(int idx) const {
        auto v = main_peaks_[static_cast<size_t>(idx) / 4];
        return {v[0][static_cast<size_t>(idx & 3)], v[1][static_cast<size_t>(idx & 3)]};
    }
private:
    void UpdateFilters();

    template <class AssignMap>
    void _UpdateFilters();

    template <class AssignMap, class Designer>
    void _UpdateFilters2();

    template <size_t kFilterNumbers, bool kOnlyPole>
    void _ProcessBlock(qwqdsp_simd_element::PackFloat<2>* main, qwqdsp_simd_element::PackFloat<2>* side,
                       int num_samples);

    float carry_w_mul_{1.0f};
    float gate_peak_{0.0f};
    float gain_{1.0f};
    FilterBankMode filter_bank_mode_;
    float sample_rate_{48000.0f};
    float freq_begin_{40.0f};
    float freq_end_{12000.0f};
    int num_bans_{16};
    int num_filters_{4};
    float attack_{1.0f};
    float release_{150.0f};
    float scale_{1.0f};
    float carry_scale_{1.0f};
    float attack_ms_{};
    float release_ms_{};
    float filter_ripple_{1.0f};
    eChannelVocoderMap map_{};
    std::array<std::pair<CascadeBPSVF, CascadeBPSVF>, global::kMaxOrder> filters_;
    std::array<qwqdsp_simd_element::PackFloat<4>[2], global::kMaxOrder> main_peaks_{};
    std::array<qwqdsp_simd_element::PackFloat<2>, 256> output_{};
};

} // namespace green_vocoder::dsp

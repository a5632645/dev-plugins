#include <qwqdsp/filter/biquad.hpp>
#include <qwqdsp/filter/svf.hpp>
#include <qwqdsp/filter/rbj.hpp>
#include <qwqdsp/filter/iir_design.hpp>
#include <array>
#include <qwqdsp/spectral/real_fft.hpp>
#include <qwqdsp/convert.hpp>

class SvfTPT {
public:
    void Reset() noexcept {
        s1_ = 0;
        s2_ = 0;
    }

    void SetAnalogPoleZero(
        qwqdsp_filter::IIRDesign::ZPK zpk, float k
    ) noexcept {
        float p_re = static_cast<float>(zpk.p.real());
        float p_im = static_cast<float>(zpk.p.imag());
        auto analog_w = std::sqrt(p_re * p_re + p_im * p_im);
        auto Q = analog_w / (-2.0f * p_re);
        Q = std::abs(Q);
        g_ = analog_w;
        R2_ = 1.0f / Q;
        d_ = 1.0f / (1.0f + R2_ * g_ + g_ * g_);

        if (zpk.z) {
            float z_re = static_cast<float>((*zpk.z).real());
            float z_im = static_cast<float>((*zpk.z).imag());
            lp_mix_ = (z_re * z_re + z_im * z_im) / (g_ * g_);
            hp_mix_ = 1;
            lp_mix_ *= (zpk.k);
            hp_mix_ *= (zpk.k);
        }
        else {
            lp_mix_ = 1;
            lp_mix_ *= (zpk.k / (g_ * g_));
            hp_mix_ = 0;
        }
    }

    float Tick(float x) noexcept {
        auto[hp, bp, lp] = TickMultiMode(x);
        return lp_mix_ * lp + hp_mix_ * hp;
    }

    /**
     * @return [hp, bp, lp]
     */
    std::array<float, 3> TickMultiMode(float x) noexcept {
        float hp = (x - (g_ + R2_) * s1_ - s2_) * d_;
        float v1 = g_ * hp;
        float bp = v1 + s1_;
        float v2 = g_ * bp;
        float lp = v2 + s2_;
        s1_ = bp + v1;
        s2_ = lp + v2;
        return {hp, bp, lp};
    }
private:
    float R2_{};
    float g_{};
    float d_{};
    float s1_{};
    float s2_{};

    float lp_mix_{};
    float hp_mix_{};
};

int main() {
    std::array<qwqdsp_filter::IIRDesign::ZPK, 4> zpk_buffer{};
    std::array<qwqdsp_filter::BiquadCoeff, 4> biquad_buffer{};
    std::array<qwqdsp_filter::IIRDesign::ZPK, 4> zpk_buffer2{};

    // qwqdsp_filter::IIRDesign::Butterworth(zpk_buffer, 2);
    // qwqdsp_filter::IIRDesign::Chebyshev1(zpk_buffer, 4, 6, false);
    // qwqdsp_filter::IIRDesign::Chebyshev2(zpk_buffer, 2, -60, false);
    qwqdsp_filter::IIRDesign::Elliptic(zpk_buffer, 2, 6, 100);
    qwqdsp_filter::IIRDesign::ProtyleToBandpass2(zpk_buffer, 2, 1, 3);

    zpk_buffer2 = zpk_buffer;

    qwqdsp_filter::IIRDesign::Bilinear(zpk_buffer, 0.5);
    qwqdsp_filter::IIRDesign::TfToBiquad(zpk_buffer, biquad_buffer);

    qwqdsp_filter::Biquad biquad;
    qwqdsp_filter::Biquad biquad2;
    qwqdsp_filter::Biquad biquad3;
    qwqdsp_filter::Biquad biquad4;
    biquad.Set(biquad_buffer[0]);
    biquad2.Set(biquad_buffer[1]);
    biquad3.Set(biquad_buffer[2]);
    biquad4.Set(biquad_buffer[3]);

    SvfTPT svf;
    SvfTPT svf2;
    SvfTPT svf3;
    SvfTPT svf4;
    svf.SetAnalogPoleZero(zpk_buffer2[0], zpk_buffer[0].k);
    svf2.SetAnalogPoleZero(zpk_buffer2[1], zpk_buffer[1].k);
    svf3.SetAnalogPoleZero(zpk_buffer2[2], zpk_buffer[2].k);
    svf4.SetAnalogPoleZero(zpk_buffer2[3], zpk_buffer[3].k);

    float ir1[8192]{1.0f};
    float ir2[8192]{1.0f};
    for (int i = 0; i < 8192; ++i) {
        ir1[i] = biquad.Tick(ir1[i]);
        ir1[i] = biquad2.Tick(ir1[i]);
        ir1[i] = biquad3.Tick(ir1[i]);
        ir1[i] = biquad4.Tick(ir1[i]);

        ir2[i] = svf.Tick(ir2[i]);
        ir2[i] = svf2.Tick(ir2[i]);
        ir2[i] = svf3.Tick(ir2[i]);
        ir2[i] = svf4.Tick(ir2[i]);
    }

    float ir1_g[4097];
    float ir2_g[4097];
    qwqdsp_spectral::RealFFT fft;
    fft.Init(8192);
    fft.FFTGainPhase(ir1, ir1_g);
    fft.FFTGainPhase(ir2, ir2_g);
    for (auto& f : ir1_g) {
        f = qwqdsp::convert::Gain2Db<-130.0f>(f);
    }
    for (auto& f : ir2_g) {
        f = qwqdsp::convert::Gain2Db<-130.0f>(f);
    }
}

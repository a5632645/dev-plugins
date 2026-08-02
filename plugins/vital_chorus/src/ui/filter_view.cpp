#include "filter_view.hpp"
#include <complex>
#include <pluginshared/component.hpp>
#include "../PluginProcessor.h"
#include "qwqdsp/convert.hpp"

namespace {

class OnePoleBilinearResponce {
public:
    void Lowpass(float w) noexcept {
        w = std::clamp(w, 0.0f, std::numbers::pi_v<float> - 1e-5f);
        auto k = std::tan(w / 2);
        b0_ = k / (1 + k);
        b1_ = b0_;
        a1_ = (k - 1) / (k + 1);
    }

    void Highpass(float w) noexcept {
        w = std::clamp(w, 0.0f, std::numbers::pi_v<float> - 1e-5f);
        auto k = std::tan(w / 2);
        b0_ = 1 / (1 + k);
        b1_ = -b0_;
        a1_ = (k - 1) / (k + 1);
    }

    std::complex<float> operator()(float w) const noexcept {
        auto z = std::polar(1.0f, w);
        auto up = z * b0_ + b1_;
        auto down = z + a1_;
        return up / down;
    }
private:
    float b0_{};
    float b1_{};
    float a1_{};
};

} // namespace

void FilterView::paint(juce::Graphics& g) {
    g.fillAll(ui::black_bg);

    float const freq_to_omega = std::numbers::pi_v<float> * 2 / static_cast<float>(p_.getSampleRate());
    float const filter_radius = p_.params_.spread.Get() * 8 * 12;
    float const low_freq = qwqdsp::convert::Pitch2Freq(p_.params_.cutoff.Get() + filter_radius);
    float const high_freq = qwqdsp::convert::Pitch2Freq(p_.params_.cutoff.Get() - filter_radius);

    OnePoleBilinearResponce lp;
    lp.Lowpass(freq_to_omega * low_freq);
    OnePoleBilinearResponce hp;
    hp.Highpass(freq_to_omega * high_freq);

    auto eval_y = [&lp, &hp, h = static_cast<float>(getHeight()),
                   fs = static_cast<float>(p_.getSampleRate())](float norm_w) {
        constexpr float pitch_begin = 8;
        constexpr float pitch_end = 136;
        float const pitch = pitch_begin + norm_w * (pitch_end - pitch_begin);
        float const w = qwqdsp::convert::Pitch2Freq(pitch) / fs * std::numbers::pi_v<float> * 2.0f;
        float gg = std::abs(lp(w) * hp(w));
        gg = qwqdsp::convert::Gain2Db<-24.0f>(gg);
        gg = std::min(gg, 0.0f);
        return juce::jmap(gg, -24.0f, 12.0f, h, 0.0f);
    };

    g.setColour(ui::line_fore);
    int const w = getWidth();
    juce::Point<float> last{0, eval_y(0.0f)};
    for (int i = 1; i < w; ++i) {
        juce::Point<float> curr{static_cast<float>(i), eval_y(static_cast<float>(i) / static_cast<float>(w))};
        g.drawLine(juce::Line<float>{last, curr});
        last = curr;
    }
}

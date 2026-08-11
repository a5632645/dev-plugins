#include "stft_vocoder.hpp"
#include <vector>
#include "PluginProcessor.h"
#include "db_curve.hpp"

namespace green_vocoder::ui {

STFTVocoder::STFTVocoder(AudioPluginAudioProcessor& processor)
    : processor_(processor) {
    bandwidth_.BindParam(processor.params_.stft_bandwidth.ptr_);
    addAndMakeVisible(bandwidth_);

    attack_.BindParam(processor.params_.stft_attack.ptr_);
    addAndMakeVisible(attack_);

    release_.BindParam(processor.params_.stft_release.ptr_);
    addAndMakeVisible(release_);

    blend_.BindParam(processor.params_.stft_blend.ptr_);
    addAndMakeVisible(blend_);

    size_.BindParam(processor.params_.stft_size.ptr_);
    addAndMakeVisible(size_);

    detail_.BindParam(processor.params_.stft_detail.ptr_);
    addAndMakeVisible(detail_);

    mfcc_size_.BindParam(processor.params_.mfcc_nbands.ptr_);
    addAndMakeVisible(mfcc_size_);
    mfcc_size_title_.setJustificationType(juce::Justification::centredLeft);
    ::ui::SetLableBlack(mfcc_size_title_);
    addAndMakeVisible(mfcc_size_title_);

    mode_.BindParam(processor.params_.stft_type.ptr_, true);
    mode_.on_value_changed = [this](size_t) { OnModeChanged(); };
    addAndMakeVisible(mode_);

    OnModeChanged();
}

void STFTVocoder::resized() {
    using enum green_vocoder::dsp::STFTMode;
    auto mode = static_cast<green_vocoder::dsp::STFTMode>(mode_.Get());

    auto b = getLocalBounds();
    auto top = b.removeFromTop(65);
    size_.setBounds(top.removeFromLeft(100).withSizeKeepingCentre(100, 30));
    attack_.setBounds(top.removeFromLeft(50));
    release_.setBounds(top.removeFromLeft(50));

    if (mode == Standard) {
        bandwidth_.setBounds(top.removeFromLeft(50));
        blend_.setBounds(top.removeFromLeft(50));
    }
    if (mode == Cepstrum) {
        detail_.setBounds(top.removeFromLeft(50).withSizeKeepingCentre(50, 65));
        blend_.setBounds(top.removeFromLeft(50));
    }
    if (mode == MFCC) {
        auto mfcc_size_bound = top.removeFromLeft(80).withSizeKeepingCentre(80, 40);
        mfcc_size_title_.setBounds(mfcc_size_bound.removeFromTop(static_cast<int>(mfcc_size_title_.getFont().getHeight())));
        mfcc_size_.setBounds(mfcc_size_bound);
    }

    mode_.setBounds(top.withHeight(30));
    {
        auto& choices = mode_.GetAllCubes();
        auto bound = mode_.getLocalBounds();
        for (auto& cube : choices) {
            cube->setBounds(bound.removeFromLeft(cube->GetTextBound(bound.getHeight())).reduced(2));
        }
    }
}

void STFTVocoder::paint(juce::Graphics& g) {
    using enum green_vocoder::dsp::STFTMode;
    switch (static_cast<green_vocoder::dsp::STFTMode>(mode_.Get())) {
        case Standard:
        case Cepstrum:
            DrawStandardCepstrum(g);
            break;
        case MFCC:
            DrawMfcc(g);
            break;
    }
}

void STFTVocoder::timerCallback() {
    repaint(getLocalBounds().removeFromTop(bandwidth_.getBottom()));
}

// -------------------- private --------------------

void STFTVocoder::DrawStandardCepstrum(juce::Graphics& g) {
    auto bb = getLocalBounds();
    bb.removeFromTop(attack_.getBottom());
    auto const plot = bb.toFloat();
    FillPlotBackground(g, plot);

    std::vector<float> gains;
    {
        juce::ScopedLock _{processor_.getCallbackLock()};
        gains = processor_.engine_.GetSTFT().GetGains();
    }

    constexpr float top_line_db = 10.0f;
    constexpr float last_line_db = -60.0f;
    constexpr float bound_top_db = 20.0f;
    constexpr float bound_bottom_db = -75.0f;
    constexpr float freq_begin = 20.0f;
    constexpr float freq_pow = 3.0f; // 20k

    DrawDbGrid(g, plot, 8, top_line_db, last_line_db, bound_top_db, bound_bottom_db);
    DrawFreqGrid(g, plot);

    // draw
    float const omega_base = freq_begin * 2.0f / static_cast<float>(processor_.engine_.GetSampleRate());
    DrawDbCurve(g, plot, omega_base, bound_top_db, bound_bottom_db, freq_pow, [&gains](float omega) -> float {
        int idx = static_cast<int>(omega * static_cast<float>(gains.size()));
        idx = std::min<int>(idx, static_cast<int>(gains.size()) - 1);
        return 20.0f * std::log10(gains[static_cast<size_t>(idx)] + 1e-10f);
    });
}

void STFTVocoder::DrawMfcc(juce::Graphics& g) {
    auto b = getLocalBounds();
    b.removeFromTop(attack_.getBottom());
    auto bb = b.toFloat();

    g.setColour(::ui::black_bg);
    g.fillRect(bb);

    constexpr float up = 0.0f;
    constexpr float down = -60.0f;

    size_t nbands = static_cast<size_t>(mfcc_size_.slider.getValue());
    float width = bb.getWidth() / static_cast<float>(nbands);
    float x = bb.getX();
    auto peaks = processor_.engine_.GetSTFT().GetMfccGains();
    for (size_t i = 0; i < nbands; ++i) {
        juce::Rectangle<float> rect{x + width * 0.25f, bb.getY(), width * 0.5f, bb.getHeight()};
        float gain = peaks[i];

        float db_gain = 20.0f * std::log10(gain + 1e-10f);
        db_gain = std::clamp(db_gain, down, up);
        float y_nor = (db_gain - (down)) / (up - (down));

        auto bin = rect.removeFromBottom(y_nor * rect.getHeight());
        g.setColour(::ui::line_fore);
        g.fillRect(bin);

        x += width;
    }
}

void STFTVocoder::OnModeChanged() {
    using enum green_vocoder::dsp::STFTMode;
    auto mode = static_cast<green_vocoder::dsp::STFTMode>(mode_.Get());
    blend_.setVisible(mode != MFCC);
    bandwidth_.setVisible(mode == Standard);
    detail_.setVisible(mode == Cepstrum);
    mfcc_size_.setVisible(mode == MFCC);
    mfcc_size_title_.setVisible(mode == MFCC);
    if (!getBounds().isEmpty()) {
        resized();
    }
}

} // namespace green_vocoder::ui

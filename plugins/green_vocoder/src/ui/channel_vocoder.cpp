#include "channel_vocoder.hpp"
#include "PluginProcessor.h"

namespace green_vocoder::ui {

ChannelVocoder::ChannelVocoder(AudioPluginAudioProcessor& p)
    : vocoder_(p.engine_.GetChannelVocoder()) {
    attack_.BindParam(p.params_.cv_attack.ptr_);
    addAndMakeVisible(attack_);
    release_.BindParam(p.params_.cv_release.ptr_);
    addAndMakeVisible(release_);
    nbands_.BindParam(p.params_.cv_nbands.ptr_);
    addAndMakeVisible(nbands_);
    nbands_title_.setJustificationType(juce::Justification::centredLeft);
    ::ui::SetLableBlack(nbands_title_);
    addAndMakeVisible(nbands_title_);
    freq_begin_.BindParam(p.params_.cv_freq_begin.ptr_);
    addAndMakeVisible(freq_begin_);
    freq_begin_title_.setJustificationType(juce::Justification::centredLeft);
    ::ui::SetLableBlack(freq_begin_title_);
    addAndMakeVisible(freq_begin_title_);
    freq_end_.BindParam(p.params_.cv_freq_end.ptr_);
    addAndMakeVisible(freq_end_);
    freq_end_title_.setJustificationType(juce::Justification::centredLeft);
    ::ui::SetLableBlack(freq_end_title_);
    addAndMakeVisible(freq_end_title_);
    scale_.BindParam(p.params_.cv_scale.ptr_);
    addAndMakeVisible(scale_);
    carry_scale_.BindParam(p.params_.cv_carry_scale.ptr_);
    addAndMakeVisible(carry_scale_);
    map_.BindParam(p.params_.cv_map.ptr_);
    addAndMakeVisible(map_);
    filter_bank_.BindParam(p.params_.cv_filter_bank_mode.ptr_);
    addAndMakeVisible(filter_bank_);
    gate_.BindParam(p.params_.cv_gate.ptr_);
    addAndMakeVisible(gate_);
    ::ui::SetLableBlack(label_filter_bank_);
    addAndMakeVisible(label_filter_bank_);
    ripple_.BindParam(p.params_.cv_ripple.ptr_);
    addAndMakeVisible(ripple_);
    ripple_title_.setJustificationType(juce::Justification::centredLeft);
    ::ui::SetLableBlack(ripple_title_);
    addAndMakeVisible(ripple_title_);
}

void ChannelVocoder::resized() {
    auto b = getLocalBounds();
    auto top = b.removeFromTop(65);
    attack_.setBounds(top.removeFromLeft(50));
    release_.setBounds(top.removeFromLeft(50));

    auto block = top.removeFromLeft(50);
    map_.setBounds(block.removeFromBottom(25));
    nbands_title_.setBounds(block.removeFromTop(static_cast<int>(nbands_title_.getFont().getHeight())));
    nbands_.setBounds(block);

    auto f_bound = top.removeFromLeft(50);
    {
        auto freq_begin_bound = f_bound.removeFromTop(f_bound.getHeight() / 2);
        freq_begin_title_.setBounds(freq_begin_bound.removeFromTop(static_cast<int>(freq_begin_title_.getFont().getHeight())));
        freq_begin_.setBounds(freq_begin_bound);
    }
    freq_end_title_.setBounds(f_bound.removeFromTop(static_cast<int>(freq_end_title_.getFont().getHeight())));
    freq_end_.setBounds(f_bound);

    scale_.setBounds(top.removeFromLeft(50));
    carry_scale_.setBounds(top.removeFromLeft(50));
    gate_.setBounds(top.removeFromLeft(50));

    auto comb = top.removeFromLeft(150);
    label_filter_bank_.setBounds(comb.removeFromTop(16));
    filter_bank_.setBounds(comb.removeFromTop(25));
    {
        auto ripple_bound = comb;
        auto ripple_width = static_cast<int>(1.2f * juce::TextLayout::getStringWidth(ripple_title_.getFont(), ripple_title_.getText()));
        ripple_title_.setBounds(ripple_bound.removeFromLeft(ripple_width));
        ripple_.setBounds(ripple_bound);
    }
}

void ChannelVocoder::paint(juce::Graphics& g) {
    auto b = getLocalBounds();
    b.removeFromTop(scale_.getBottom());
    auto bb = b.toFloat();

    g.setColour(::ui::black_bg);
    g.fillRect(bb);

    constexpr float up = 5.0f;
    constexpr float down = -60.0f;

    int nbands = vocoder_.GetNumBins();
    float width = static_cast<float>(bb.getWidth()) / static_cast<float>(nbands);
    float x = bb.getX();
    for (int i = 0; i < nbands; ++i) {
        juce::Rectangle<float> rect{x + width * 0.25f, bb.getY(), width * 0.5f, bb.getHeight()};
        float gain = vocoder_.GetBinPeak(static_cast<size_t>(i))[0];

        float db_gain = 20.0f * std::log10(gain + 1e-10f);
        db_gain = std::clamp(db_gain, down, up);
        float y_nor = (db_gain - (down)) / (up - (down));

        auto bin = rect.removeFromBottom(y_nor * rect.getHeight());
        g.setColour(::ui::line_fore);
        g.fillRect(bin);

        x += width;
    }
}

} // namespace green_vocoder::ui

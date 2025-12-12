#include "channel_vocoder.hpp"
#include "PluginProcessor.h"
#include "param_ids.hpp"

namespace green_vocoder::widget {

ChannelVocoder::ChannelVocoder(AudioPluginAudioProcessor& p)
    : vocoder_(p.channel_vocoder_) {
    auto& apvts = *p.value_tree_;

    attack_.BindParam(apvts, id::kChannelVocoderAttack);
    addAndMakeVisible(attack_);
    release_.BindParam(apvts, id::kChannelVocoderRelease);
    addAndMakeVisible(release_);
    nbands_.BindParam(apvts, id::kChannelVocoderNBands);
    addAndMakeVisible(nbands_);
    freq_begin_.BindParam(apvts, id::kChannelVocoderFreqBegin);
    addAndMakeVisible(freq_begin_);
    freq_end_.BindParam(apvts, id::kChannelVocoderFreqEnd);
    addAndMakeVisible(freq_end_);
    scale_.BindParam(apvts, id::kChannelVocoderScale);
    addAndMakeVisible(scale_);
    carry_scale_.BindParam(apvts, id::kChannelVocoderCarryScale);
    addAndMakeVisible(carry_scale_);
    map_.BindParam(apvts, id::kChannelVocoderMap);
    addAndMakeVisible(map_);
    filter_bank_.BindParam(apvts, id::kChannelVocoderFilterBankMode);
    addAndMakeVisible(filter_bank_);
    gate_.BindParam(apvts, id::kChannelVocoderGate);
    addAndMakeVisible(gate_);
    ui::SetLableBlack(label_filter_bank_);
    addAndMakeVisible(label_filter_bank_);
}

void ChannelVocoder::resized() {
    auto b = getLocalBounds();
    auto top = b.removeFromTop(65);
    attack_.setBounds(top.removeFromLeft(50));
    release_.setBounds(top.removeFromLeft(50));
    
    auto block = top.removeFromLeft(50);
    map_.setBounds(block.removeFromBottom(25));
    nbands_.setBounds(block);

    auto f_bound = top.removeFromLeft(50);
    freq_begin_.setBounds(f_bound.removeFromTop(f_bound.getHeight() / 2));
    freq_end_.setBounds(f_bound);

    scale_.setBounds(top.removeFromLeft(50));
    carry_scale_.setBounds(top.removeFromLeft(50));
    gate_.setBounds(top.removeFromLeft(50));

    auto comb = top.removeFromLeft(150);
    label_filter_bank_.setBounds(comb.removeFromTop(20));
    filter_bank_.setBounds(comb.removeFromTop(30));
}

void ChannelVocoder::paint(juce::Graphics& g) {
    auto b = getLocalBounds();
    b.removeFromTop(scale_.getBottom());
    auto bb = b.toFloat();

    g.setColour(ui::black_bg);
    g.fillRect(bb);

    constexpr float up = 20.0f;
    constexpr float down = -60.0f;

    int nbands = vocoder_.GetNumBins();
    float width = bb.getWidth() / nbands;
    float x = bb.getX();
    for (int i = 0; i < nbands; ++i) {
        juce::Rectangle<float> rect{ x + width * 0.25f, bb.getY(), width * 0.5f, bb.getHeight() };
        float gain = vocoder_.GetBinPeak(i)[0];

        float db_gain = 20.0f * std::log10(gain + 1e-10f);
        db_gain = std::clamp(db_gain, down, up);
        float y_nor = (db_gain - (down)) / (up - (down));

        auto bin = rect.removeFromBottom(y_nor * rect.getHeight());
        g.setColour(ui::line_fore);
        g.fillRect(bin);

        x += width;
    }
}


}

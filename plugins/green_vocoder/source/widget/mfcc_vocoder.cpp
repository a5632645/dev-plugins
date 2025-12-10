#include "mfcc_vocoder.hpp"
#include "PluginProcessor.h"
#include "param_ids.hpp"

namespace green_vocoder::widget {

MFCCVocoder::MFCCVocoder(AudioPluginAudioProcessor& p)
    : p_(p) {
    auto& apvts = *p.value_tree_;

    attack_.BindParam(apvts, id::kStftAttack);
    addAndMakeVisible(attack_);
    release_.BindParam(apvts, id::kStftRelease);
    addAndMakeVisible(release_);
    fft_size_.BindParam(apvts, id::kStftSize);
    addAndMakeVisible(fft_size_);
    mfcc_size_.BindParam(apvts, id::kMfccNumBands);
    addAndMakeVisible(mfcc_size_);
}

void MFCCVocoder::resized() {
    auto b = getLocalBounds();

    auto block = b.removeFromLeft(100).withHeight(65);
    fft_size_.setBounds(block.removeFromTop(30));
    mfcc_size_.setBounds(block);

    auto top = b.removeFromTop(65);
    attack_.setBounds(top.removeFromLeft(50));
    release_.setBounds(top.removeFromLeft(50));
}

void MFCCVocoder::paint(juce::Graphics& g) {
    auto b = getLocalBounds();
    b.removeFromTop(attack_.getBottom());
    auto bb = b.toFloat();

    g.setColour(ui::black_bg);
    g.fillRect(bb);

    constexpr float up = 0.0f;
    constexpr float down = -60.0f;

    size_t nbands = static_cast<size_t>(mfcc_size_.slider.getValue());
    float width = bb.getWidth() / static_cast<float>(nbands);
    float x = bb.getX();
    auto peaks = p_.mfcc_vocoder_.gains_;
    for (size_t i = 0; i < nbands; ++i) {
        juce::Rectangle<float> rect{ x + width * 0.25f, bb.getY(), width * 0.5f, bb.getHeight() };
        float gain = peaks[i];

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

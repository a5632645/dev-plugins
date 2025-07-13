#include "burg_lpc.hpp"
#include "../PluginProcessor.h"
#include "../param_ids.hpp"
#include "tooltips.hpp"
#include <numbers>

namespace widget {

BurgLPC::BurgLPC(AudioPluginAudioProcessor& processor)
: processor_(processor) {
    auto& apvts = *processor.value_tree_;

    addAndMakeVisible(lpc_label_);

    lpc_foorget_.BindParameter(apvts, id::kForgetRate);
    addAndMakeVisible(lpc_foorget_);
    lpc_smooth_.BindParameter(apvts, id::kLPCSmooth);
    addAndMakeVisible(lpc_smooth_);
    lpc_dicimate_.BindParameter(apvts, id::kLPCDicimate);
    addAndMakeVisible(lpc_dicimate_);
    lpc_order_.BindParameter(apvts, id::kLPCOrder);
    addAndMakeVisible(lpc_order_);
    lpc_attack_.BindParameter(apvts, id::kLPCGainAttack);
    addAndMakeVisible(lpc_attack_);
    lpc_release_.BindParameter(apvts, id::kLPCGainRelease);
    addAndMakeVisible(lpc_release_);

    tooltip::tooltips.AddListenerAndInvoke(this);
}

void BurgLPC::OnLanguageChanged(tooltip::Tooltips& strs) {
    lpc_label_.setText(strs.Label(id::combbox::kVocoderNameIds[0]), juce::NotificationType::dontSendNotification);
}

void BurgLPC::resized() {
    auto b = getLocalBounds();
    lpc_label_.setBounds(b.removeFromTop(20));
    auto top = b.removeFromTop(100);
    lpc_foorget_.setBounds(top.removeFromLeft(50));
    lpc_smooth_.setBounds(top.removeFromLeft(50));
    lpc_dicimate_.setBounds(top.removeFromLeft(50));
    lpc_order_.setBounds(top.removeFromLeft(50));
    lpc_attack_.setBounds(top.removeFromLeft(50));
    lpc_release_.setBounds(top.removeFromLeft(50));
}

void BurgLPC::paint(juce::Graphics& g) {
    auto bb = getLocalBounds();
    bb.removeFromTop(lpc_release_.getBottom());

    g.setColour(juce::Colours::black);
    g.fillRect(bb);

    int w = bb.getWidth();
    auto b = bb.toFloat();

    // lattice to tf
    std::array<float, dsp::BurgLPC::kNumPoles> lattice_buff;
    std::array<float, dsp::BurgLPC::kNumPoles> transfer_function;
    processor_.burg_lpc_.CopyLatticeCoeffient(lattice_buff);
    int order = processor_.burg_lpc_.GetOrder();
    for (int i = 0; i < order; ++i) {
        transfer_function[i] = lattice_buff[i];
        for (int j = 0; j < i - 1; ++j) {
            transfer_function[j] = transfer_function[j] - lattice_buff[i] * transfer_function[i - j];
        }
    }

    constexpr float up = 30.0f;
    constexpr float down = -30.0f;
    // draw
    juce::Point<float> line_last{ b.getX(), b.getCentreY() };
    g.setColour(juce::Colours::green);
    float mul_val = std::pow(10.0f, 2.0f / w);
    float mul_begin = 1.0f;
    float omega_base = 180.0f * std::numbers::pi_v<float> / static_cast<float>(processor_.getSampleRate());
    for (int x = 0; x < w; ++x) {
        // float omega = static_cast<float>(x) * std::numbers::pi_v<float> / static_cast<float>(w - 1);
        float omega = omega_base * mul_begin;
        mul_begin *= mul_val;
        auto z_responce = std::complex{1.0f, 0.0f};
        for (int i = 0; i < order; ++i) {
            auto z = std::polar(1.0f, -omega * (i + 1));
            z_responce -= transfer_function[i] * z;
        }
        z_responce = 1.0f / z_responce;
        if (std::isnan(z_responce.real()) || std::isnan(z_responce.imag())) {
            continue;
        }

        float gain = std::abs(z_responce);
        float db_gain = 20.0f * std::log10(gain + 1e-10f);
        if (db_gain < down) db_gain = down;
        float y_nor = (db_gain - (down)) / (up - (down));
        float y = b.getBottom() - y_nor * b.getHeight();
        juce::Point line_end{ static_cast<float>(x + b.toFloat().getX()), y };
        g.drawLine(juce::Line<float>{line_last, line_end}, 2.0f);
        line_last = line_end;
    }

    g.setColour(juce::Colours::white);
    g.drawRect(bb);
}

void BurgLPC::timerCallback() {
    repaint(getLocalBounds().removeFromTop(lpc_release_.getBottom()));
}

}

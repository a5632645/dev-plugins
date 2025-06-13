#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "juce_graphics/juce_graphics.h"
#include "param_ids.hpp"
#include <algorithm>
#include <array>
#include <complex>
#include <numbers>

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    auto& apvts = *p.value_tree_;

    addAndMakeVisible(filter_);
    hp_pitch_.BindParameter(apvts, id::kHighpassPitch);
    hp_pitch_.SetShortName("HPASS");
    addAndMakeVisible(hp_pitch_);
    em_pitch_.BindParameter(apvts, id::kEmphasisPitch);
    em_pitch_.SetShortName("HSHELF");
    addAndMakeVisible(em_pitch_);
    em_gain_.BindParameter(apvts, id::kEmphasisGain);
    em_gain_.SetShortName("GAIN");
    addAndMakeVisible(em_gain_);
    em_s_.BindParameter(apvts, id::kEmphasisS);
    em_s_.SetShortName("S");
    addAndMakeVisible(em_s_);

    addAndMakeVisible(shifter_);
    shift_pitch_.BindParameter(apvts, id::kShiftPitch);
    shift_pitch_.SetShortName("PITCH");
    addAndMakeVisible(shift_pitch_);

    addAndMakeVisible(lpc_);
    lpc_learn_.BindParameter(apvts, id::kLearnRate);
    lpc_learn_.SetShortName("LEARN");
    addAndMakeVisible(lpc_learn_);
    lpc_foorget_.BindParameter(apvts, id::kForgetRate);
    lpc_foorget_.SetShortName("FORGET");
    addAndMakeVisible(lpc_foorget_);
    lpc_smooth_.BindParameter(apvts, id::kLPCSmooth);
    lpc_smooth_.SetShortName("SMOOTH");
    addAndMakeVisible(lpc_smooth_);
    lpc_order_.BindParameter(apvts, id::kLPCOrder);
    lpc_order_.SetShortName("ORDER");
    addAndMakeVisible(lpc_order_);
    lpc_attack_.BindParameter(apvts, id::kLPCGainAttack);
    lpc_attack_.SetShortName("ATTACK");
    addAndMakeVisible(lpc_attack_);
    lpc_release_.BindParameter(apvts, id::kLPCGainRelease);
    lpc_release_.SetShortName("RELEASE");
    addAndMakeVisible(lpc_release_);

    addAndMakeVisible(stft_);
    stft_bandwidth_.BindParameter(apvts, id::kStftWindowWidth);
    stft_bandwidth_.SetShortName("BANDW");
    addAndMakeVisible(stft_bandwidth_);

    setSize (500, 600);
    startTimerHz(30);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor() {
    stopTimer();
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g) {
    g.fillAll(juce::Colours::grey);
    auto bb = getLocalBounds();
    bb.removeFromTop(stft_bandwidth_.getBottom());

    g.setColour(juce::Colours::black);
    g.fillRect(bb);
    g.setColour(juce::Colours::white);
    g.drawRect(bb);

    int w = bb.getWidth();
    auto b = bb.toFloat();

    // // lattice to tf
    // // std::array<float, dsp::BackLPC::kNumPoles> lattice_buff;
    // std::array<float, dsp::BackLPC::kNumPoles> transfer_function;
    // // processorRef.lpc_.CopyLatticeCoeffient(lattice_buff);
    // // int order = processorRef.lpc_.GetOrder();
    // // for (int i = 0; i < order; ++i) {
    // //     transfer_function[i] = lattice_buff[i];
    // //     for (int j = 0; j < i - 1; ++j) {
    // //         transfer_function[j] = transfer_function[j] - lattice_buff[i] * transfer_function[i - j];
    // //     }
    // // }
    // processorRef.rls_lpc_.CopyLatticeCoeffient(transfer_function);
    // int order = processorRef.rls_lpc_.GetOrder();

    // constexpr float up = 60.0f;
    // constexpr float down = -20.0f;
    // // draw
    // int w = getWidth();
    // juce::Point<float> line_last{ b.getX(), b.getCentreY() };
    // g.setColour(juce::Colours::green);
    // float mul_val = std::pow(10.0f, 2.0f / w);
    // float mul_begin = 1.0f;
    // float omega_base = 180.0f * std::numbers::pi_v<float> / processorRef.getSampleRate();
    // for (int x = 0; x < w; ++x) {
    //     // float omega = static_cast<float>(x) * std::numbers::pi_v<float> / static_cast<float>(w - 1);
    //     float omega = omega_base * mul_begin;
    //     mul_begin *= mul_val;
    //     auto z_responce = std::complex{1.0f, 0.0f};
    //     for (int i = 0; i < order; ++i) {
    //         auto z = std::polar(1.0f, -omega * (i + 1));
    //         z_responce -= transfer_function[i] * z;
    //     }
    //     z_responce = 1.0f / z_responce;
    //     if (std::isnan(z_responce.real()) || std::isnan(z_responce.imag())) {
    //         continue;
    //     }

    //     float gain = std::abs(z_responce);
    //     float db_gain = 20.0f * std::log10(gain + 1e-8f);
    //     if (db_gain < down) db_gain = down;
    //     float y_nor = (db_gain - (down)) / (up - (down));
    //     float y = b.getBottom() - y_nor * b.getHeight();
    //     juce::Point line_end{ static_cast<float>(x + b.toFloat().getX()), y };
    //     g.drawLine(juce::Line<float>{line_last, line_end}, 2.0f);
    //     line_last = line_end;
    // }
}

void AudioPluginAudioProcessorEditor::resized() {
    auto b = getLocalBounds();
    {
        filter_.setBounds(b.removeFromTop(20));
        auto top = b.removeFromTop(100);
        hp_pitch_.setBounds(top.removeFromLeft(50));
        em_pitch_.setBounds(top.removeFromLeft(50));
        em_gain_.setBounds(top.removeFromLeft(50));
        em_s_.setBounds(top.removeFromLeft(50));
    }
    {
        shifter_.setBounds(b.removeFromTop(20));
        auto top = b.removeFromTop(100);
        shift_pitch_.setBounds(top.removeFromLeft(50));
    }
    {
        lpc_.setBounds(b.removeFromTop(20));
        auto top = b.removeFromTop(100);
        lpc_learn_.setBounds(top.removeFromLeft(50));
        lpc_foorget_.setBounds(top.removeFromLeft(50));
        lpc_smooth_.setBounds(top.removeFromLeft(50));
        lpc_order_.setBounds(top.removeFromLeft(50));
        lpc_attack_.setBounds(top.removeFromLeft(50));
        lpc_release_.setBounds(top.removeFromLeft(50));
    }
    {
        stft_.setBounds(b.removeFromTop(20));
        auto top = b.removeFromTop(100);
        stft_bandwidth_.setBounds(top.removeFromLeft(50));
    }
}

void AudioPluginAudioProcessorEditor::timerCallback() {
    repaint();
}
#include "vocoder.hpp"
#include "burg_lpc.hpp"
#include "channel_vocoder.hpp"
#include "stft_vocoder.hpp"
#include "mfcc_vocoder.hpp"
#include "PluginProcessor.h"

namespace green_vocoder::widget {
Vocoder::Vocoder(AudioPluginAudioProcessor& p) {
    auto& apvts = *p.value_tree_;

    addAndMakeVisible(title_);
    shift_pitch_.BindParam(apvts, id::kShiftPitch);
    addAndMakeVisible(shift_pitch_);
    vocoder_type_.BindParam(apvts, id::kVocoderType);
    vocoder_type_.addListener(this);
    addAndMakeVisible(vocoder_type_);

    burg_ = std::make_unique<widget::BurgLPC>(p);
    channel_ = std::make_unique<widget::ChannelVocoder>(p);
    stft_ = std::make_unique<widget::STFTVocoder>(p);
    mfcc_ = std::make_unique<widget::MFCCVocoder>(p);
    addChildComponent(burg_.get());
    addChildComponent(channel_.get());
    addChildComponent(stft_.get());
    addChildComponent(mfcc_.get());

    comboBoxChanged(&vocoder_type_);
    startTimerHz(30);
}

Vocoder::~Vocoder() {
    current_vocoder_widget_ = nullptr;
    burg_ = nullptr;
    channel_ = nullptr;
    stft_ = nullptr;
    mfcc_ = nullptr;
}

void Vocoder::resized() {
    auto b = getLocalBounds();
    title_.setBounds(b.removeFromTop(20));

    auto top = b.removeFromTop(30);
    shift_pitch_.setBounds(top.removeFromLeft(150));
    vocoder_type_.setBounds(top.removeFromLeft(150));

    burg_->setBounds(b);
    channel_->setBounds(b);
    stft_->setBounds(b);
    mfcc_->setBounds(b);
}

void Vocoder::comboBoxChanged(juce::ComboBox* box) {
    if (box == &vocoder_type_) {
        if (current_vocoder_widget_ != nullptr) {
            current_vocoder_widget_->setVisible(false);
        }

        auto type = static_cast<eVocoderType>(vocoder_type_.getSelectedItemIndex());
        switch (type) {
            case eVocoderType_LeakyBurgLPC:
            case eVocoderType_BlockBurgLPC:
                current_vocoder_widget_ = burg_.get();
                static_cast<widget::BurgLPC*>(current_vocoder_widget_)->SetBlockMode(type == eVocoderType_BlockBurgLPC);
                break;
            case eVocoderType_STFTVocoder:
                current_vocoder_widget_ = stft_.get();
                break;
            case eVocoderType_MFCCVocoder:
                current_vocoder_widget_ = mfcc_.get();
                break;
            case eVocoderType_ChannelVocoder:
                current_vocoder_widget_ = channel_.get();
                break;
            default:
                jassertfalse;
                break;
        }

        if (current_vocoder_widget_ != nullptr) {
            current_vocoder_widget_->setVisible(true);
        }
    }
}

void Vocoder::timerCallback() {
    if (current_vocoder_widget_ != nullptr) {
        current_vocoder_widget_->repaint();
    }
}
}

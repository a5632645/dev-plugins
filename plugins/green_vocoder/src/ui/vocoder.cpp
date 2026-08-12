#include "vocoder.hpp"
#include "PluginProcessor.h"
#include "burg_lpc.hpp"
#include "channel_vocoder.hpp"
#include "stft_vocoder.hpp"

namespace green_vocoder::ui {
Vocoder::Vocoder(AudioPluginAudioProcessor& p) {
    addAndMakeVisible(title_);
    shift_pitch_.BindParam(p.params_.shift_pitch.ptr_);
    addAndMakeVisible(shift_pitch_);
    shift_pitch_title_.setJustificationType(juce::Justification::centredLeft);
    ::ui::SetLableBlack(shift_pitch_title_);
    addAndMakeVisible(shift_pitch_title_);
    vocoder_type_.BindParam(p.params_.vocoder_type.ptr_, true);
    vocoder_type_.on_value_changed = [this](int) { OnVocoderTypeChanged(); };
    addAndMakeVisible(vocoder_type_);

    burg_ = std::make_unique<BurgLPC>(p);
    channel_ = std::make_unique<ChannelVocoder>(p);
    stft_ = std::make_unique<STFTVocoder>(p);
    addChildComponent(burg_.get());
    addChildComponent(channel_.get());
    addChildComponent(stft_.get());

    OnVocoderTypeChanged();
}

Vocoder::~Vocoder() {
    current_vocoder_widget_ = nullptr;
    burg_ = nullptr;
    channel_ = nullptr;
    stft_ = nullptr;
}

void Vocoder::resized() {
    auto b = getLocalBounds();
    title_.setBounds(b.removeFromTop(20));

    auto top = b.removeFromTop(30);
    {
        auto shift_pitch_bound = top.removeFromLeft(150).reduced(2);
        auto shift_pitch_width = static_cast<int>(1.2f * juce::TextLayout::getStringWidth(shift_pitch_title_.getFont(), shift_pitch_title_.getText()));
        shift_pitch_title_.setBounds(shift_pitch_bound.removeFromLeft(shift_pitch_width));
        shift_pitch_.setBounds(shift_pitch_bound);
    }
    top.removeFromLeft(8);
    vocoder_type_.setBounds(top);
    {
        auto bound = vocoder_type_.getLocalBounds();
        auto& cubes = vocoder_type_.GetAllCubes();
        for (auto& cube : cubes) {
            cube->setBounds(bound.removeFromLeft(cube->GetTextBound(bound.getHeight())).reduced(2));
        }
    }

    burg_->setBounds(b);
    channel_->setBounds(b);
    stft_->setBounds(b);
}

void Vocoder::OnVocoderTypeChanged() {
    if (current_vocoder_widget_ != nullptr) {
        current_vocoder_widget_->setVisible(false);
    }

    auto type = static_cast<eVocoderType>(vocoder_type_.Get());
    switch (type) {
        case eVocoderType_LeakyBurgLPC:
        case eVocoderType_BlockBurgLPC:
            current_vocoder_widget_ = burg_.get();
            static_cast<BurgLPC*>(current_vocoder_widget_)->SetBlockMode(type == eVocoderType_BlockBurgLPC);
            break;
        case eVocoderType_STFTVocoder:
            current_vocoder_widget_ = stft_.get();
            break;
        case eVocoderType_ChannelVocoder:
            current_vocoder_widget_ = channel_.get();
            break;
        case eVocoderType_NumVocoderTypes:
        default:
            jassertfalse;
            break;
    }

    if (current_vocoder_widget_ != nullptr) {
        current_vocoder_widget_->setVisible(true);
    }
}

} // namespace green_vocoder::ui

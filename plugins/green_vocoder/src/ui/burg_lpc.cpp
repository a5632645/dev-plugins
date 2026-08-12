#include "burg_lpc.hpp"
#include "../PluginProcessor.h"

namespace green_vocoder::ui {

BurgLPC::BurgLPC(AudioPluginAudioProcessor& processor)
    : processor_(processor) {
    forget_.BindParam(processor.params_.lpc_forget.ptr_);
    addAndMakeVisible(forget_);
    smear_.BindParam(processor.params_.lpc_smooth.ptr_);
    addAndMakeVisible(smear_);
    order_.BindParam(processor.params_.lpc_order.ptr_);
    addAndMakeVisible(order_);
    order_title_.setJustificationType(juce::Justification::centredLeft);
    ::ui::SetLableBlack(order_title_);
    addAndMakeVisible(order_title_);
    attack_.BindParam(processor.params_.lpc_gain_attack.ptr_);
    addAndMakeVisible(attack_);
    hold_.BindParam(processor.params_.lpc_gain_hold.ptr_);
    addAndMakeVisible(hold_);
    release_.BindParam(processor.params_.lpc_gain_release.ptr_);
    addAndMakeVisible(release_);

    block_size_.BindParam(processor.params_.stft_size.ptr_);
    addChildComponent(block_size_);

    MakeGui();
}

void BurgLPC::resized() {
    auto b = getLocalBounds();
    if (!block_mode_) {
        auto top = b.removeFromTop(65);
        forget_.setBounds(top.removeFromLeft(50));
        smear_.setBounds(top.removeFromLeft(50));
        auto block = top.removeFromLeft(60);
        auto order_bound = block.withHeight(40);
        order_title_.setBounds(order_bound.removeFromTop(static_cast<int>(order_title_.getFont().getHeight())));
        order_.setBounds(order_bound);
        attack_.setBounds(top.removeFromLeft(50));
        hold_.setBounds(top.removeFromLeft(50));
        release_.setBounds(top.removeFromLeft(50));
    }
    else {
        block_size_.setBounds(b.removeFromLeft(80).withHeight(65).withSizeKeepingCentre(80, 35).reduced(2));
        auto order_bound = b.removeFromLeft(80).withHeight(40).reduced(2);
        order_title_.setBounds(order_bound.removeFromTop(static_cast<int>(order_title_.getFont().getHeight())));
        order_.setBounds(order_bound);
        auto block = b.removeFromTop(65);
        smear_.setBounds(block.removeFromLeft(50));
        attack_.setBounds(block.removeFromLeft(50));
    }
}

void BurgLPC::SetBlockMode(bool block_mode) {
    block_mode_ = block_mode;
    MakeGui();
}

void BurgLPC::MakeGui() {
    forget_.setVisible(!block_mode_);
    release_.setVisible(!block_mode_);
    hold_.setVisible(!block_mode_);
    block_size_.setVisible(block_mode_);
    resized();
}

} // namespace green_vocoder::ui

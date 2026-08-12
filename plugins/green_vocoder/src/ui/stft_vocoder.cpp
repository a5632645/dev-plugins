#include "stft_vocoder.hpp"
#include "PluginProcessor.h"

namespace green_vocoder::ui {

// 顶部控件区高度（左侧两个下拉框 1 列 2 行 + 右侧 dial 行）
constexpr int kTopHeight = 65;

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

    smooth_type_.BindParam(processor.params_.stft_smooth_erb.ptr_);
    addAndMakeVisible(smooth_type_);

    smooth_.BindParam(processor.params_.stft_smooth.ptr_);
    addAndMakeVisible(smooth_);

    welch_.BindParam(processor.params_.stft_welch.ptr_);
    addAndMakeVisible(welch_);

    floor_.BindParam(processor.params_.stft_floor.ptr_);
    addAndMakeVisible(floor_);

    morph_.BindParam(processor.params_.stft_morph.ptr_);
    addAndMakeVisible(morph_);

    direction_.BindParam(processor.params_.stft_morph_ab.ptr_);
    addAndMakeVisible(direction_);

    wiener_glitch_.BindParam(processor.params_.stft_wiener_glitch.ptr_);
    addAndMakeVisible(wiener_glitch_);

    wiener_ab_.BindParam(processor.params_.stft_wiener_ab.ptr_);
    addAndMakeVisible(wiener_ab_);

    mfcc_size_.BindParam(processor.params_.mfcc_nbands.ptr_);
    addAndMakeVisible(mfcc_size_);

    mode_.BindParam(processor.params_.stft_type.ptr_);
    mode_.onChange = [this] { OnModeChanged(); };
    addAndMakeVisible(mode_);

    OnModeChanged();
}

void STFTVocoder::resized() {
    using enum green_vocoder::dsp::STFTMode;
    auto mode = static_cast<green_vocoder::dsp::STFTMode>(mode_.getSelectedItemIndex());

    auto b = getLocalBounds();
    auto top = b.removeFromTop(kTopHeight);

    // 左侧 1 列 2 行：块大小 / 算法两个下拉框
    auto left = top.removeFromLeft(100);
    size_.setBounds(left.removeFromTop(30).withSizeKeepingCentre(100, 30));
    mode_.setBounds(left.withHeight(30).withSizeKeepingCentre(100, 30));

    // 右侧：逐一放置可见控件（隐藏控件不占空间）
    // dial 统一 50×65；switch 按内容排版
    auto place = [&top](juce::Component& c, int width, int height) {
        if (c.isVisible())
            c.setBounds(top.removeFromLeft(width).withSizeKeepingCentre(width, height));
    };
    auto place_dial = [&place](juce::Component& c) { place(c, 50, 65); };
    auto place_switch = [&place](juce::Component& c) { place(c, 70, 30); };

    if (mode == Wiener) {
        // Wiener 无 attack/release/blend
        place_switch(wiener_glitch_);
        place_switch(wiener_ab_);
    }
    else if (mode == Morph) {
        // Morph 无 attack/release
        place_dial(morph_);
        place_switch(direction_);
    }
    else {
        place_dial(attack_);
        place_dial(release_);
        if (mode == Standard) {
            place_dial(bandwidth_);
            place_dial(blend_);
        }
        else if (mode == Cepstrum) {
            place_dial(detail_);
            place_dial(blend_);
        }
        else if (mode == MFCC) {
            place_dial(mfcc_size_);
            place_dial(blend_);
        }
        else if (mode == Smooth) {
            place_switch(smooth_type_);
            place_dial(smooth_);
            place_dial(blend_);
        }
        else if (mode == Welch) {
            place_dial(welch_);
            place_dial(floor_);
            place_dial(blend_);
        }
    }
}

// -------------------- private --------------------

void STFTVocoder::OnModeChanged() {
    using enum green_vocoder::dsp::STFTMode;
    auto mode = static_cast<green_vocoder::dsp::STFTMode>(mode_.getSelectedItemIndex());
    attack_.setVisible(mode != Morph && mode != Wiener);
    release_.setVisible(mode != Morph && mode != Wiener);
    blend_.setVisible(mode != Morph && mode != Wiener);
    bandwidth_.setVisible(mode == Standard);
    welch_.setVisible(mode == Welch);
    floor_.setVisible(mode == Welch);
    morph_.setVisible(mode == Morph);
    direction_.setVisible(mode == Morph);
    wiener_glitch_.setVisible(mode == Wiener);
    wiener_ab_.setVisible(mode == Wiener);
    detail_.setVisible(mode == Cepstrum);
    smooth_type_.setVisible(mode == Smooth);
    smooth_.setVisible(mode == Smooth);
    mfcc_size_.setVisible(mode == MFCC);
    if (!getBounds().isEmpty()) {
        resized();
    }
}

} // namespace green_vocoder::ui

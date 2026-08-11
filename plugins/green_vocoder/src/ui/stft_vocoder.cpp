#include "stft_vocoder.hpp"
#include "PluginProcessor.h"
#include "db_curve.hpp"
#include <vector>

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

    wiener_variant_.BindParam(processor.params_.stft_wiener_variant.ptr_);
    addAndMakeVisible(wiener_variant_);

    wiener_snr_.BindParam(processor.params_.stft_wiener_snr.ptr_);
    addAndMakeVisible(wiener_snr_);

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
        place_switch(wiener_variant_);
        place_dial(wiener_snr_);
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

void STFTVocoder::paint(juce::Graphics& g) {
    using enum green_vocoder::dsp::STFTMode;
    switch (static_cast<green_vocoder::dsp::STFTMode>(mode_.getSelectedItemIndex())) {
        case Standard:
        case Cepstrum:
        case Smooth:
        case Welch:
        case Morph:
        case Wiener:
            DrawStandardCepstrum(g);
            break;
        case MFCC:
            DrawMfcc(g);
            break;
    }
}

void STFTVocoder::timerCallback() {
    repaint(getLocalBounds().removeFromTop(kTopHeight));
}

// -------------------- private --------------------

void STFTVocoder::DrawStandardCepstrum(juce::Graphics& g) {
    auto bb = getLocalBounds();
    bb.removeFromTop(kTopHeight);
    auto const plot = bb.toFloat();
    FillPlotBackground(g, plot);

    std::vector<float> gains;
    {
        juce::ScopedLock _{processor_.getCallbackLock()};
        gains = processor_.engine_.GetSTFT().GetGains();
    }

    // Morph 输出幅度（软限幅后）可高于 20 dB，抬高绘图上限与顶部网格线避免曲线被顶部截断
    const bool is_morph =
        static_cast<green_vocoder::dsp::STFTMode>(mode_.getSelectedItemIndex()) == green_vocoder::dsp::STFTMode::Morph;
    const float bound_top_db = is_morph ? 45.0f : 15.0f;
    const float top_line_db = is_morph ? 40.0f : 10.0f;
    constexpr float last_line_db = -60.0f;
    constexpr float bound_bottom_db = -65.0f;
    constexpr float freq_begin = 20.0f;
    constexpr float freq_pow = 3.0f; // 20k

    DrawDbGrid(g, plot, 5, top_line_db, last_line_db, bound_top_db, bound_bottom_db);
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
    b.removeFromTop(kTopHeight);
    auto bb = b.toFloat();

    g.setColour(::ui::black_bg);
    g.fillRect(bb);

    constexpr float up = 10.0f;
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

    // 水平 dB 网格（0 ~ -60 dB，每 10 dB 一条，带刻度）
    DrawDbGrid(g, bb, 7, up, down, up + 5.0f, down - 5.0f);
}

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
    wiener_variant_.setVisible(mode == Wiener);
    wiener_snr_.setVisible(mode == Wiener);
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

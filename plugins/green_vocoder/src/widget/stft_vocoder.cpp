#include "stft_vocoder.hpp"
#include <vector>
#include "PluginProcessor.h"
#include "param_ids.hpp"

namespace green_vocoder::widget {

STFTVocoder::STFTVocoder(AudioPluginAudioProcessor& processor)
    : processor_(processor) {
    auto& apvts = *processor.value_tree_;

    bandwidth_.BindParam(apvts, id::kStftWindowWidth);
    addAndMakeVisible(bandwidth_);

    attack_.BindParam(apvts, id::kStftAttack);
    addAndMakeVisible(attack_);

    release_.BindParam(apvts, id::kStftRelease);
    addAndMakeVisible(release_);

    blend_.BindParam(apvts, id::kStftBlend);
    addAndMakeVisible(blend_);

    size_.BindParam(apvts, id::kStftSize);
    addAndMakeVisible(size_);

    detail_.BindParam(apvts, id::kStftDetail);
    addAndMakeVisible(detail_);

    mfcc_size_.BindParam(apvts, id::kMfccNumBands);
    addAndMakeVisible(mfcc_size_);

    mode_.BindParam(apvts, id::kStftType, true);
    mode_.on_value_changed = [this](size_t index) {
        OnModeChanged();
    };
    addAndMakeVisible(mode_);

    OnModeChanged();
}

void STFTVocoder::resized() {
    using enum green_vocoder::dsp::STFTVocoder::Mode;
    auto mode = static_cast<green_vocoder::dsp::STFTVocoder::Mode>(mode_.Get());

    auto b = getLocalBounds();
    auto top = b.removeFromTop(65);
    size_.setBounds(top.removeFromLeft(100).withSizeKeepingCentre(100, 30));
    attack_.setBounds(top.removeFromLeft(50));
    release_.setBounds(top.removeFromLeft(50));
    
    if (mode == Standard) {
        bandwidth_.setBounds(top.removeFromLeft(50));
        blend_.setBounds(top.removeFromLeft(50));
    }
    if (mode == Cepstrum) {
        detail_.setBounds(top.removeFromLeft(50).withSizeKeepingCentre(50, 65));
        blend_.setBounds(top.removeFromLeft(50));
    }
    if (mode == MFCC) {
        mfcc_size_.setBounds(top.removeFromLeft(80).withSizeKeepingCentre(80, 40));
    }

    mode_.setBounds(top.withHeight(30));
    {
        auto& choices = mode_.GetAllCubes();
        auto bound = mode_.getLocalBounds();
        for (auto& cube : choices) {
            cube->setBounds(bound.removeFromLeft(cube->GetTextBound(bound.getHeight())).reduced(2));
        }
    }
}

void STFTVocoder::paint(juce::Graphics& g) {
    using enum green_vocoder::dsp::STFTVocoder::Mode;
    switch (static_cast<green_vocoder::dsp::STFTVocoder::Mode>(mode_.Get())) {
        case Standard:
        case Cepstrum:
            DrawStandardCepstrum(g);
            break;
        case MFCC:
            DrawMfcc(g);
            break;
    }
}

void STFTVocoder::timerCallback() {
    repaint(getLocalBounds().removeFromTop(bandwidth_.getBottom()));
}

// -------------------- private --------------------

void STFTVocoder::DrawStandardCepstrum(juce::Graphics& g) {
    auto bb = getLocalBounds();
    bb.removeFromTop(attack_.getBottom());
    g.setColour(ui::black_bg);
    g.fillRect(bb);
    auto current_font = g.getCurrentFont();
    std::vector<float> gains;
    {
        juce::ScopedLock _{processor_.getCallbackLock()};
        gains = processor_.stft_vocoder_.gains_;
    }

    constexpr float top_line_db = 10.0f;
    constexpr float last_line_db = -60.0f;
    constexpr float bound_top_db = 20.0f;
    constexpr float bound_bottom_db = -75.0f;
    constexpr float freq_begin = 20.0f;
    constexpr float freq_pow = 3.0f; // 20k
    auto convert_db_to_y = [y = bb.getY(), h = bb.getHeight()](float db) -> float {
        if (db < bound_bottom_db)
            return static_cast<float>(y + h);
        else if (db > bound_top_db)
            return static_cast<float>(y);
        auto nor = (db - bound_bottom_db) / (bound_top_db - bound_bottom_db);
        return y + h * (1.0f - nor);
    };
    // draw lines
    {
        constexpr int nlines = 8;
        constexpr float db_span = (top_line_db - last_line_db) / (nlines - 1.0f);
        g.setColour(juce::Colours::grey);
        for (int i = 0; i < nlines; ++i) {
            auto db = last_line_db + db_span * i;
            auto y = convert_db_to_y(db);
            g.drawHorizontalLine(y, bb.getX(), bb.getRight());
            g.drawSingleLineText(std::to_string(static_cast<int>(db)), bb.getX(),
                                 y + g.getCurrentFont().getHeight() / 2);
        }
    }
    {
        // 1~9 * base -> 0.0~1.0(<1.0)
        static const std::array kLogJtable{
            0.0f,
            std::log10(2.0f),
            std::log10(3.0f),
            std::log10(4.0f),
            std::log10(5.0f),
            std::log10(6.0f),
            std::log10(7.0f),
            std::log10(8.0f),
            std::log10(9.0f),
        };
        static const juce::StringArray kFreqStr{"20", "200", "2k", "20k"};
        float w = bb.getWidth();
        float span_w = w / 3.0f;
        for (int i = 0; i < 3; ++i) {
            float span_x = span_w * i;
            for (int j = 0; j < 9; ++j) {
                float log_nor = kLogJtable[j];
                float x = span_x + span_w * log_nor;
                g.drawVerticalLine(x, bb.getY(), bb.getBottom());
            }
            if (i == 0) {
                g.drawSingleLineText(kFreqStr[i], span_x, bb.getBottom() - current_font.getHeight() / 2);
            }
            else {
                auto str_w = juce::TextLayout::getStringWidth(current_font, kFreqStr[i]);
                g.drawSingleLineText(kFreqStr[i], span_x - str_w / 2, bb.getBottom() - current_font.getHeight() / 2);
            }
        }
        // 绘制最后的频率
        auto last_w = juce::TextLayout::getStringWidth(current_font, kFreqStr[3]);
        g.drawSingleLineText(kFreqStr[3], bb.getRight() - last_w, bb.getBottom() - current_font.getHeight() / 2);
    }

    auto b = bb.toFloat();
    juce::Point<float> line_last{b.getX(), b.getCentreY()};
    g.setColour(ui::line_fore);
    float mul_val = std::pow(10.0f, freq_pow / b.getWidth());
    float mul_begin = 1.0f;
    float omega_base = freq_begin * 2.0f / static_cast<float>(processor_.getSampleRate());
    for (int x = 0; x < bb.getWidth(); ++x) {
        float omega = omega_base * mul_begin;
        mul_begin *= mul_val;

        int idx = static_cast<int>(omega * gains.size());
        idx = std::min<int>(idx, static_cast<int>(gains.size()) - 1);
        float gain = gains[idx];
        float db_gain = 20.0f * std::log10(gain + 1e-10f);
        float y = convert_db_to_y(db_gain);
        juce::Point line_end{static_cast<float>(x + b.toFloat().getX()), y};
        g.drawLine(juce::Line<float>{line_last, line_end}, 2.0f);
        line_last = line_end;
    }
}

void STFTVocoder::DrawMfcc(juce::Graphics& g) {
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
    auto peaks = processor_.stft_vocoder_.mfcc_gains_;
    for (size_t i = 0; i < nbands; ++i) {
        juce::Rectangle<float> rect{x + width * 0.25f, bb.getY(), width * 0.5f, bb.getHeight()};
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

void STFTVocoder::OnModeChanged() {
    using enum green_vocoder::dsp::STFTVocoder::Mode;
    auto mode = static_cast<green_vocoder::dsp::STFTVocoder::Mode>(mode_.Get());
    blend_.setVisible(mode != MFCC);
    bandwidth_.setVisible(mode == Standard);
    detail_.setVisible(mode == Cepstrum);
    mfcc_size_.setVisible(mode == MFCC);
    if (!getBounds().isEmpty()) {
        resized();
    }
}

} // namespace green_vocoder::widget

#include "stft_vocoder.hpp"
#include "PluginProcessor.h"
#include "param_ids.hpp"
#include <vector>

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

    use_v2_.BindParam(apvts, id::kStftVocoderV2);
    addAndMakeVisible(use_v2_);
    detail_.BindParam(apvts, id::kStftDetail);
    addAndMakeVisible(detail_);

    use_v2_.onClick = [this] {
        bool display = use_v2_.getToggleState();
        detail_.setVisible(display);
    };
    use_v2_.onClick();
}

void STFTVocoder::resized() {
    auto b = getLocalBounds();
    auto top = b.removeFromTop(65);
    bandwidth_.setBounds(top.removeFromLeft(50));
    attack_.setBounds(top.removeFromLeft(50));
    release_.setBounds(top.removeFromLeft(50));
    blend_.setBounds(top.removeFromLeft(50));
    size_.setBounds(top.removeFromLeft(100).withSizeKeepingCentre(100, 30));
    top.removeFromLeft(4);
    use_v2_.setBounds(top.removeFromLeft(50).withSizeKeepingCentre(50, 30));
    detail_.setBounds(top.removeFromLeft(50).withSizeKeepingCentre(50, 65));
}

void STFTVocoder::paint(juce::Graphics& g) {
    auto bb = getLocalBounds();
    bb.removeFromTop(bandwidth_.getBottom());
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
    auto convert_db_to_y = [y = bb.getY(), h = bb.getHeight()](float db) ->float {
        if (db < bound_bottom_db) return static_cast<float>(y + h);
        else if (db > bound_top_db) return static_cast<float>(y);
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
            g.drawSingleLineText(std::to_string(static_cast<int>(db)), bb.getX(), y + g.getCurrentFont().getHeight() / 2);
        }
    }
    {
        // 1~9 * base -> 0.0~1.0(<1.0)
        static const std::array kLogJtable {
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
        static const juce::StringArray kFreqStr {
            "20",
            "200",
            "2k",
            "20k"
        };
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
    juce::Point<float> line_last{ b.getX(), b.getCentreY() };
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
        juce::Point line_end{ static_cast<float>(x + b.toFloat().getX()), y };
        g.drawLine(juce::Line<float>{line_last, line_end}, 2.0f);
        line_last = line_end;
    }

    // g.setColour(juce::Colours::white);
    // g.drawRect(bb);
}

void STFTVocoder::timerCallback() {
    repaint(getLocalBounds().removeFromTop(bandwidth_.getBottom()));
}

}

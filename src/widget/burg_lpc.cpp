#include "burg_lpc.hpp"
#include "../PluginProcessor.h"
#include "../param_ids.hpp"
#include "juce_core/juce_core.h"
#include "juce_graphics/juce_graphics.h"
#include "tooltips.hpp"
#include <numbers>
#include <string>

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
}

void BurgLPC::OnLanguageChanged(tooltip::Tooltips& strs) {
    lpc_label_.setText(strs.Label(id::combbox::kVocoderNameIds[0]), juce::NotificationType::dontSendNotification);
    lpc_foorget_.OnLanguageChanged(strs);
    lpc_smooth_.OnLanguageChanged(strs);
    lpc_dicimate_.OnLanguageChanged(strs);
    lpc_order_.OnLanguageChanged(strs);
    lpc_attack_.OnLanguageChanged(strs);
    lpc_release_.OnLanguageChanged(strs);
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
    auto current_font = g.getCurrentFont();

    constexpr float top_line_db = 80.0f;
    constexpr float last_line_db = -20.0f;
    constexpr float bound_top_db = 85.0f;
    constexpr float bound_bottom_db = -25.0f;
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
        constexpr int nlines = 5;
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
            float span_x = span_w * i + bb.getX();
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

    // lattice to tf
    std::array<float, dsp::BurgLPC::kNumPoles> lattice_buff;
    std::array<float, dsp::BurgLPC::kNumPoles + 1> upgoing{1};
    std::array<float, dsp::BurgLPC::kNumPoles + 1> downgoing{1};

    processor_.burg_lpc_.CopyLatticeCoeffient(lattice_buff);
    size_t order = processor_.burg_lpc_.GetOrder();

    for (size_t kidx = 0; kidx < order; ++kidx) {
        for (size_t i = kidx + 1; i != 0; --i) {
            downgoing[i] = downgoing[i - 1];
        }
        downgoing[0] = 0;

        for (size_t i = 0; i < kidx + 2; ++i) {
            float up = upgoing[i] + lattice_buff[kidx] * downgoing[i];
            float down = downgoing[i] + lattice_buff[kidx] * upgoing[i];
            upgoing[i] = up;
            downgoing[i] = down;
        }
    }

    // draw
    int w = bb.getWidth();
    auto b = bb.toFloat();
    juce::Point<float> line_last{ b.getX(), b.getCentreY() };
    g.setColour(juce::Colours::green);
    float mul_val = std::pow(10.0f, freq_pow / w);
    float mul_begin = 1.0f;
    float omega_base = freq_begin * std::numbers::pi_v<float> / static_cast<float>(processor_.getSampleRate());
    for (int x = 0; x < w; ++x) {
        float omega = omega_base * mul_begin;
        mul_begin *= mul_val;

        auto z_responce = std::complex{1.0f, 0.0f};
        for (size_t i = 0; i < order; ++i) {
            auto z = std::polar(1.0f, -omega * (i + 1));
            z_responce += upgoing[i + 1] * z;
        }
        z_responce = 1.0f / z_responce;
        if (std::isnan(z_responce.real()) || std::isnan(z_responce.imag())) {
            continue;
        }

        float gain = std::abs(z_responce);
        float db_gain = 20.0f * std::log10(gain + 1e-10f);
        float y = convert_db_to_y(db_gain);
        juce::Point line_end{ static_cast<float>(x + b.toFloat().getX()), y };
        g.drawLine(juce::Line<float>{line_last, line_end}, 2.0f);
        line_last = line_end;
    }

    g.setColour(juce::Colours::white);
    g.drawRect(bb);
}

}

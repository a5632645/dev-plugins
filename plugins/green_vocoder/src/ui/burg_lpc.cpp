#include "burg_lpc.hpp"
#include <numbers>
#include <string>
#include "../global.hpp"
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

void BurgLPC::paint(juce::Graphics& g) {
    auto bb = getLocalBounds();
    bb.removeFromTop(65);
    g.setColour(::ui::black_bg);
    g.fillRect(bb);
    auto current_font = g.getCurrentFont();

    constexpr float top_line_db = 80.0f;
    constexpr float last_line_db = -20.0f;
    constexpr float bound_top_db = 85.0f;
    constexpr float bound_bottom_db = -25.0f;
    constexpr float freq_begin = 20.0f;
    constexpr float freq_pow = 3.0f; // 20k
    auto convert_db_to_y = [y = bb.getY(), h = bb.getHeight()](float db) -> float {
        if (db < bound_bottom_db)
            return static_cast<float>(y + h);
        else if (db > bound_top_db)
            return static_cast<float>(y);
        auto nor = (db - bound_bottom_db) / (bound_top_db - bound_bottom_db);
        return static_cast<float>(y) + static_cast<float>(h) * (1.0f - nor);
    };
    // draw lines
    {
        constexpr int nlines = 5;
        constexpr float db_span = (top_line_db - last_line_db) / (nlines - 1.0f);
        g.setColour(::ui::grid_fore);
        for (int i = 0; i < nlines; ++i) {
            auto db = last_line_db + db_span * static_cast<float>(i);
            auto y = convert_db_to_y(db);
            g.drawHorizontalLine(static_cast<int>(y), static_cast<float>(bb.getX()), static_cast<float>(bb.getRight()));
            g.drawSingleLineText(std::to_string(static_cast<int>(db)), bb.getX(),
                                 static_cast<int>(y + g.getCurrentFont().getHeight() / 2));
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
        float w = static_cast<float>(bb.getWidth());
        float span_w = w / 3.0f;
        for (int i = 0; i < 3; ++i) {
            float span_x = span_w * static_cast<float>(i) + static_cast<float>(bb.getX());
            for (int j = 0; j < 9; ++j) {
                float log_nor = kLogJtable[static_cast<size_t>(j)];
                float x = span_x + span_w * log_nor;
                g.drawVerticalLine(static_cast<int>(x), static_cast<float>(bb.getY()),
                                   static_cast<float>(bb.getBottom()));
            }
            if (i == 0) {
                g.drawSingleLineText(kFreqStr[i], static_cast<int>(span_x),
                                     static_cast<int>(bb.getBottom()) - static_cast<int>(current_font.getHeight() / 2));
            }
            else {
                auto str_w = juce::TextLayout::getStringWidth(current_font, kFreqStr[i]);
                g.drawSingleLineText(kFreqStr[i], static_cast<int>(span_x - str_w / 2),
                                     static_cast<int>(bb.getBottom()) - static_cast<int>(current_font.getHeight() / 2));
            }
        }
        // 绘制最后的频率
        auto last_w = juce::TextLayout::getStringWidth(current_font, kFreqStr[3]);
        g.drawSingleLineText(kFreqStr[3], static_cast<int>(bb.getRight()) - static_cast<int>(last_w),
                             static_cast<int>(bb.getBottom()) - static_cast<int>(current_font.getHeight() / 2));
    }

    // lattice to tf
    std::array<float, global::kNumPoles> lattice_buff;
    std::array<float, global::kNumPoles + 1> upgoing{1};
    std::array<float, global::kNumPoles + 1> downgoing{1};

    int order = static_cast<int>(order_.slider.getValue());
    ;
    if (block_mode_) {
        processor_.engine_.GetBlockBurgLPC().CopyLatticeCoeffient(lattice_buff, order);
    }
    else {
        processor_.engine_.GetBurgLPC().CopyLatticeCoeffient(lattice_buff, order);
    }

    for (int kidx = 0; kidx < order; ++kidx) {
        for (int i = kidx + 1; i != 0; --i) {
            downgoing[static_cast<size_t>(i)] = downgoing[static_cast<size_t>(i) - 1];
        }
        downgoing[0] = 0;

        for (int i = 0; i < kidx + 2; ++i) {
            float up = upgoing[static_cast<size_t>(i)] + lattice_buff[static_cast<size_t>(kidx)] * downgoing[static_cast<size_t>(i)];
            float down = downgoing[static_cast<size_t>(i)] + lattice_buff[static_cast<size_t>(kidx)] * upgoing[static_cast<size_t>(i)];
            upgoing[static_cast<size_t>(i)] = up;
            downgoing[static_cast<size_t>(i)] = down;
        }
    }

    // draw
    int w = bb.getWidth();
    auto b = bb.toFloat();
    juce::Point<float> line_last{b.getX(), b.getCentreY()};
    g.setColour(::ui::line_fore);
    float mul_val = std::pow(10.0f, freq_pow / static_cast<float>(w));
    float mul_begin = 1.0f;
    float omega_base = freq_begin * std::numbers::pi_v<float> / static_cast<float>(processor_.engine_.GetSampleRate());
    for (int x = 0; x < w; ++x) {
        float omega = omega_base * mul_begin;
        mul_begin *= mul_val;

        auto z_responce = std::complex{1.0f, 0.0f};
        for (int i = 0; i < order; ++i) {
            auto z = std::polar(1.0f, -omega * static_cast<float>(i + 1));
            z_responce += upgoing[static_cast<size_t>(i) + 1] * z;
        }
        z_responce = 1.0f / z_responce;
        if (std::isnan(z_responce.real()) || std::isnan(z_responce.imag())) {
            continue;
        }

        float gain = std::abs(z_responce);
        float db_gain = 20.0f * std::log10(gain + 1e-10f);
        float y = convert_db_to_y(db_gain);
        juce::Point line_end{static_cast<float>(x) + b.toFloat().getX(), y};
        g.drawLine(juce::Line<float>{line_last, line_end}, 2.0f);
        line_last = line_end;
    }

    // g.setColour(juce::Colours::white);
    // g.drawRect(bb);
}

} // namespace green_vocoder::ui

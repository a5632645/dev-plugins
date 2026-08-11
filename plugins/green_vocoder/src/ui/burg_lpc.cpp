#include "burg_lpc.hpp"
#include "../PluginProcessor.h"
#include "../global.hpp"
#include "db_curve.hpp"
#include <complex>
#include <limits>
#include <numbers>

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
    auto const plot = bb.toFloat();
    FillPlotBackground(g, plot);

    constexpr float top_line_db = 80.0f;
    constexpr float last_line_db = -20.0f;
    constexpr float bound_top_db = 85.0f;
    constexpr float bound_bottom_db = -25.0f;
    constexpr float freq_begin = 20.0f;
    constexpr float freq_pow = 3.0f; // 20k

    DrawDbGrid(g, plot, 5, top_line_db, last_line_db, bound_top_db, bound_bottom_db);
    DrawFreqGrid(g, plot);

    // lattice to tf
    std::array<float, global::kNumPoles> lattice_buff;
    std::array<float, global::kNumPoles + 1> upgoing{1};
    std::array<float, global::kNumPoles + 1> downgoing{1};

    int order = static_cast<int>(order_.slider.getValue());
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
            float up = upgoing[static_cast<size_t>(i)]
                     + lattice_buff[static_cast<size_t>(kidx)] * downgoing[static_cast<size_t>(i)];
            float down = downgoing[static_cast<size_t>(i)]
                       + lattice_buff[static_cast<size_t>(kidx)] * upgoing[static_cast<size_t>(i)];
            upgoing[static_cast<size_t>(i)] = up;
            downgoing[static_cast<size_t>(i)] = down;
        }
    }

    // draw
    float const omega_base =
        freq_begin * std::numbers::pi_v<float> / static_cast<float>(processor_.engine_.GetSampleRate());
    DrawDbCurve(g, plot, omega_base, bound_top_db, bound_bottom_db, freq_pow, [&upgoing, order](float omega) -> float {
        auto z_responce = std::complex{1.0f, 0.0f};
        auto z_pow = std::complex{1.0f, 0.0f};
        auto const z_step = std::polar(1.0f, -omega);
        for (int i = 0; i < order; ++i) {
            z_pow *= z_step; // 增量幂：z^(i+1)，避免逐阶 polar(sin/cos)
            z_responce += upgoing[static_cast<size_t>(i) + 1] * z_pow;
        }
        z_responce = 1.0f / z_responce;
        if (std::isnan(z_responce.real()) || std::isnan(z_responce.imag()))
            return std::numeric_limits<float>::quiet_NaN();
        return 20.0f * std::log10(std::abs(z_responce) + 1e-10f);
    });

    // g.setColour(juce::Colours::white);
    // g.drawRect(bb);
}

} // namespace green_vocoder::ui

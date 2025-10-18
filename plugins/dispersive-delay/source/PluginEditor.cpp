#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "dsp/convert.hpp"

// ---------------------------------------- editor ----------------------------------------

DispersiveDelayAudioProcessorEditor::DispersiveDelayAudioProcessorEditor (DispersiveDelayAudioProcessor& p)
    : AudioProcessorEditor (&p)
    , p_(p)
    , preset_panel_(*p.preset_manager_)
{
    auto& apvt = *p.value_tree_;
    addAndMakeVisible(preset_panel_);

    group_delay_cache_.resize(256);

    delay_time_.BindParam(apvt, "delay_time");
    delay_time_.setHelpText("delay time, unit is ms");
    delay_time_.slider.onValueChange = [this] {
        TryUpdateGroupDelay();
    };

    f_begin_.BindParam(apvt, "f_begin");
    f_begin_.setHelpText("frequency begin, unit is semitone");
    f_begin_.slider.onValueChange = [this] {
        TryUpdateGroupDelay();
    };

    f_end_.BindParam(apvt, "f_end");
    f_end_.setHelpText("frequency end, unit is semitone");
    f_end_.slider.onValueChange = [this] {
        TryUpdateGroupDelay();
    };

    flat_.BindParam(apvt, "flat");
    flat_.setHelpText("control the all pass filter pole radius behavior");
    flat_.slider.onValueChange = [this] {
        TryUpdateGroupDelay();
    };

    min_bw_.BindParam(apvt, "min_bw");
    min_bw_.slider.onValueChange = [this] {
        TryUpdateGroupDelay();
    };

    x_axis_.setButtonText("pitch-x");
    x_axis_.setTooltip("if enable, x axis is pitch unit. otherwise, x axis is hz unit");
    x_axis_attachment_ = std::make_unique<juce::ButtonParameterAttachment>(*p.pitch_x_asix_, x_axis_);
    x_axis_.onStateChange = [this] {
        TryUpdateGroupDelay();
    };

    ui::SetLableBlack(res_label_);
    res_label_.setText("resolution", juce::dontSendNotification);
    reslution_.addItemList(p.resolution_->choices, 1);
    resolution_attachment_ = std::make_unique<juce::ComboBoxParameterAttachment>(*p.resolution_, reslution_);
    reslution_.onChange = [this] {
        TryUpdateGroupDelay();
    };

    panic_.setButtonText("panic");
    panic_.onClick = [this] {
        p_.PanicFilterFb();
    };

    clear_curve_.setButtonText("clear");
    clear_curve_.onClick = [this] {
        p_.curve_->Init(mana::CurveV2::CurveInitEnum::kRamp);
    };

    feedback_.BindParam(apvt, "feedback");
    delay_.BindParam(apvt, "delay");
    damp_.BindParam(apvt, "damp");
    addAndMakeVisible(feedback_);
    addAndMakeVisible(delay_);
    addAndMakeVisible(damp_);

    addAndMakeVisible(delay_time_);
    addAndMakeVisible(f_begin_);
    addAndMakeVisible(f_end_);
    addAndMakeVisible(flat_);
    addAndMakeVisible(min_bw_);
    addAndMakeVisible(curve_);
    ui::SetLableBlack(num_filter_label_);
    addAndMakeVisible(num_filter_label_);
    addAndMakeVisible(x_axis_);
    addAndMakeVisible(reslution_);
    addAndMakeVisible(res_label_);
    addAndMakeVisible(clear_curve_);
    addAndMakeVisible(panic_);

    setSize (720, 360 + 50);
    setResizable(true, true);
    setResizeLimits(720, 360 + 50, 9999, 9999);

    curve_.SetCurve(p_.curve_.get());
    curve_.SetSnapGrid(true);
    curve_.SetGridNum(16, 8);

    p_.curve_->AddListener(this);
    TryUpdateGroupDelay();
}


DispersiveDelayAudioProcessorEditor::~DispersiveDelayAudioProcessorEditor() {
    p_.curve_->RemoveListener(this);
    resolution_attachment_ = nullptr;
    x_axis_attachment_ = nullptr;
}

//==============================================================================
void DispersiveDelayAudioProcessorEditor::paint (juce::Graphics& g) {
    g.fillAll(ui::green_bg);
}

void DispersiveDelayAudioProcessorEditor::paintOverChildren(juce::Graphics& g)
{
    auto max_delay_ms = *std::ranges::max_element(group_delay_cache_);
    max_delay_ms = std::max(max_delay_ms, 0.001f);
    auto inverse_max_delay_ms = 1.0f / max_delay_ms;
    {
        auto b = curve_.GetComponentBounds().withY(curve_.getY() + 10);
        auto size = group_delay_cache_.capacity();
        juce::Path p;

        for (size_t i = 0; i < size; ++i) {
            auto nor_x = i / static_cast<float>(size);
            auto nor_y = group_delay_cache_[i] * inverse_max_delay_ms;
            auto x = b.getX() + b.getWidth() * nor_x;
            auto y = b.getY() + b.getHeight() * (1.0f - nor_y);
            if (i == 0) {
                p.startNewSubPath(x, y);
            }
            else {
                p.lineTo(x, y);
            }
        }

        g.setColour(juce::Colours::red);
        g.strokePath(p, juce::PathStrokeType(1.0f));
    }
    {
        auto b = curve_.GetComponentBounds().withY(curve_.getY() + 10);
        auto begin = p_.f_begin_->get();
        auto end = p_.f_end_->get();
        auto x_begin = b.getX() + b.getWidth() * begin;
        auto x_end = b.getX() + b.getWidth() * end;
        g.setColour(juce::Colours::lightblue);
        g.drawVerticalLine(x_begin, b.getY(), b.getBottom());
        g.drawVerticalLine(x_end, b.getY(), b.getBottom());
    }
}

void DispersiveDelayAudioProcessorEditor::resized() {
    auto b = getLocalBounds();
    preset_panel_.setBounds(b.removeFromTop(50));
    {
        auto slider_aera = b.removeFromTop(20+25*3);
        flat_.setBounds(slider_aera.removeFromLeft(64));
        f_begin_.setBounds(slider_aera.removeFromLeft(64));
        f_end_.setBounds(slider_aera.removeFromLeft(64));
        delay_time_.setBounds(slider_aera.removeFromLeft(64));
        min_bw_.setBounds(slider_aera.removeFromLeft(64));
        {
            auto buttons_aera = slider_aera.removeFromLeft(std::min(slider_aera.getWidth(), 180));
            {
                auto res_aera = buttons_aera.removeFromTop(20);
                res_label_.setBounds(res_aera.removeFromLeft(80));
                reslution_.setBounds(res_aera.withWidth(100));
            }
            {
                auto btn_aera = buttons_aera.removeFromRight(80);
                btn_aera.removeFromTop(4);
                clear_curve_.setBounds(btn_aera.removeFromTop(25));
                btn_aera.removeFromTop(4);
                panic_.setBounds(btn_aera.removeFromTop(25));
            }
            x_axis_.setBounds(buttons_aera.removeFromTop(30).reduced(4, 0));
            num_filter_label_.setBounds(buttons_aera);
        }
        {
            slider_aera.removeFromLeft(8);
            feedback_.setBounds(slider_aera.removeFromLeft(64));
            delay_.setBounds(slider_aera.removeFromLeft(64));
            damp_.setBounds(slider_aera.removeFromLeft(64));
        }
    }
    curve_.setBounds(b);
}

void DispersiveDelayAudioProcessorEditor::TryUpdateGroupDelay() {
    num_filter_label_.setText(juce::String{ "n.filters: " } + juce::String(p_.delays_.GetNumFilters()), juce::dontSendNotification);

    constexpr auto pi = std::numbers::pi_v<float>;
    auto fs = static_cast<float>(p_.getSampleRate());

    group_delay_cache_.clear();
    auto size = group_delay_cache_.capacity();
    if (x_axis_.getToggleState()) {
        for (size_t i = 0; i < size; ++i) {
            auto nor = i / static_cast<float>(size);
            auto hz = MelMap(nor);
            auto w = hz / fs * 2 * pi;
            auto delay_num_samples = p_.delays_.GetGroupDelay(w);
            auto delay_num_ms = delay_num_samples * 1000.0f / fs;
            group_delay_cache_.emplace_back(delay_num_ms);
        }
    }
    else {
        for (size_t i = 0; i < size; ++i) {
            auto nor = i / static_cast<float>(size);
            auto hz = std::lerp(20.0f, 20000.0f, nor);
            auto w = hz / fs * 2 * pi;
            auto delay_num_samples = p_.delays_.GetGroupDelay(w);
            auto delay_num_ms = delay_num_samples * 1000.0f / fs;
            group_delay_cache_.emplace_back(delay_num_ms);
        }
    }
    repaint();
}

void DispersiveDelayAudioProcessorEditor::OnAddPoint(mana::CurveV2* generator, mana::CurveV2::Point p, int before_idx) {
    TryUpdateGroupDelay();
}

void DispersiveDelayAudioProcessorEditor::OnRemovePoint(mana::CurveV2* generator, int remove_idx) {
    TryUpdateGroupDelay();
}

void DispersiveDelayAudioProcessorEditor::OnPointXyChanged(mana::CurveV2* generator, int changed_idx) {
    TryUpdateGroupDelay();
}

void DispersiveDelayAudioProcessorEditor::OnPointPowerChanged(mana::CurveV2* generator, int changed_idx) {
    TryUpdateGroupDelay();
}

void DispersiveDelayAudioProcessorEditor::OnReload(mana::CurveV2* generator) {
    TryUpdateGroupDelay();
}

#include "PluginEditor.h"
#include "PluginProcessor.h"

// ---------------------------------------- editor ----------------------------------------

DispersiveDelayAudioProcessorEditor::DispersiveDelayAudioProcessorEditor (DispersiveDelayAudioProcessor& p)
    : AudioProcessorEditor (&p)
    , p_(p)
{
        auto& apvt = *p.value_tree_;

    delay_time_.setText("time");
    delay_time_.ParamLink(apvt, "delay_time");
    delay_time_.setHelpText("delay time, unit is ms");

    f_begin_.setText("f_begin");
    f_begin_.ParamLink(apvt, "f_begin");
    f_begin_.setHelpText("frequency begin, unit is semitone");

    f_end_.setText("f_end");
    f_end_.ParamLink(apvt, "f_end");
    f_end_.setHelpText("frequency end, unit is semitone");

    beta_.setText("flat");
    beta_.ParamLink(apvt, "flat");
    beta_.setHelpText("control the all pass filter pole radius behavior");

    min_bw_.setText("min_bw");
    min_bw_.ParamLink(apvt, "min_bw");

    x_axis_.setButtonText("pitch-x");
    x_axis_.setTooltip("if enable, x axis is pitch unit. otherwise, x axis is hz unit");
    x_axis_attachment_ = std::make_unique<juce::ButtonParameterAttachment>(*p.pitch_x_asix_, x_axis_);

    res_label_.setText("resolution", juce::dontSendNotification);
    reslution_.addItemList(p.resolution_->choices, 1);
    resolution_attachment_ = std::make_unique<juce::ComboBoxParameterAttachment>(*p.resolution_, reslution_);

    random_.setButtonText("random");
    random_.onClick = [this] {
        p_.RandomParameter();
    };

    panic_.setButtonText("panic");
    panic_.onClick = [this] {
        p_.PanicFilterFb();
    };

    clear_curve_.setButtonText("clear");
    clear_curve_.onClick = [this] {
        p_.curve_->Init(mana::CurveV2::CurveInitEnum::kRamp);
    };

    addAndMakeVisible(delay_time_);
    addAndMakeVisible(f_begin_);
    addAndMakeVisible(f_end_);
    addAndMakeVisible(beta_);
    addAndMakeVisible(min_bw_);
    addAndMakeVisible(curve_);
    addAndMakeVisible(num_filter_label_);
    addAndMakeVisible(x_axis_);
    addAndMakeVisible(reslution_);
    addAndMakeVisible(res_label_);
    addAndMakeVisible(random_);
    addAndMakeVisible(clear_curve_);
    addAndMakeVisible(panic_);

    setSize (500, 300);
    setResizable(true, true);
    setResizeLimits(500, 300, 9999, 9999);

    curve_.SetCurve(p_.curve_.get());
    curve_.SetSnapGrid(true);
    curve_.SetGridNum(16, 8);

    group_delay_cache_.resize(256);
    startTimerHz(10);
}


DispersiveDelayAudioProcessorEditor::~DispersiveDelayAudioProcessorEditor() {
    resolution_attachment_ = nullptr;
    x_axis_attachment_ = nullptr;
}

//==============================================================================
void DispersiveDelayAudioProcessorEditor::paint (juce::Graphics& g) {
    g.fillAll(juce::Colour{22,27,32});

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
    {
        auto slider_aera = b.removeFromTop(20+25*3);
        beta_.setBounds(slider_aera.removeFromLeft(64));
        f_begin_.setBounds(slider_aera.removeFromLeft(64));
        f_end_.setBounds(slider_aera.removeFromLeft(64));
        delay_time_.setBounds(slider_aera.removeFromLeft(64));
        min_bw_.setBounds(slider_aera.removeFromLeft(64));
        {
            {
                auto res_aera = slider_aera.removeFromTop(20);
                res_label_.setBounds(res_aera.removeFromLeft(80));
                reslution_.setBounds(res_aera);
            }
            {
                auto btn_aera = slider_aera.removeFromRight(80);
                random_.setBounds(btn_aera.removeFromTop(25));
                clear_curve_.setBounds(btn_aera.removeFromTop(25));
                panic_.setBounds(btn_aera);
            }
            x_axis_.setBounds(slider_aera.removeFromTop(20));
            num_filter_label_.setBounds(slider_aera);
        }
    }
    curve_.setBounds(b);
}

void DispersiveDelayAudioProcessorEditor::timerCallback()
{
    constexpr auto pi = std::numbers::pi_v<float>;
    auto fs = static_cast<float>(p_.getSampleRate());

    group_delay_cache_.clear();
    auto size = group_delay_cache_.capacity();
    for (size_t i = 0; i < size; ++i) {
        auto nor = i / static_cast<float>(size);
        auto hz = SemitoneMap(nor);
        auto w = hz / fs * 2 * pi;
        auto delay_num_samples = p_.delays_[0].GetGroupDelay(w);
        auto delay_num_ms = delay_num_samples * 1000.0f / fs;
        group_delay_cache_.emplace_back(delay_num_ms);
    }

    num_filter_label_.setText(juce::String{ "n.filters: " } + juce::String(p_.delays_[0].GetNumFilters()), juce::dontSendNotification);
    repaint();
}
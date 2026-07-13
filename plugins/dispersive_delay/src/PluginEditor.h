#pragma once
#include "pluginshared/component.hpp"
#include "pluginshared/preset_panel.hpp"

#include "ui/common_curve_editor.h"

// ---------------------------------------- editor ----------------------------------------

class DispersiveDelayAudioProcessor;

//==============================================================================
class DispersiveDelayAudioProcessorEditor final 
    : public juce::AudioProcessorEditor
    , public mana::CurveV2::Listener {
public:
    explicit DispersiveDelayAudioProcessorEditor (DispersiveDelayAudioProcessor&);
    ~DispersiveDelayAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void paintOverChildren (juce::Graphics&) override;

private:
    void TryUpdateGroupDelay();
    // implement for Listener
    void OnAddPoint(mana::CurveV2* generator, mana::CurveV2::Point p, int before_idx) override;
    void OnRemovePoint(mana::CurveV2* generator, int remove_idx) override;
    void OnPointXyChanged(mana::CurveV2* generator, int changed_idx) override;
    void OnPointPowerChanged(mana::CurveV2* generator, int changed_idx) override;
    void OnReload(mana::CurveV2* generator) override;

    DispersiveDelayAudioProcessor& p_;
    pluginshared::PresetPanel preset_panel_;

    ui::Dial flat_{"flat"};
    ui::Dial f_begin_{"begin"};
    ui::Dial f_end_{"end"};
    ui::Dial delay_time_{"delay"};
    ui::Dial min_bw_{"min_bw"};
    mana::CommonCurveEditor curve_;
    juce::Label num_filter_label_;
    ui::Switch x_axis_{"mel", "hz"};
    std::unique_ptr<juce::ButtonParameterAttachment> x_axis_attachment_;

    juce::Label res_label_;
    juce::ComboBox reslution_;
    std::unique_ptr<juce::ComboBoxParameterAttachment> resolution_attachment_;

    ui::FlatButton clear_curve_;
    ui::FlatButton panic_;

    ui::Dial feedback_{"feedback"};
    ui::Dial delay_{"delay"};
    ui::Dial damp_{"damp"};

    std::vector<float> group_delay_cache_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DispersiveDelayAudioProcessorEditor)
};

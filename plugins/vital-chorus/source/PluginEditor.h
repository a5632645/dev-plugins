#pragma once
#include "../../shared/component.hpp"

class VitalChorusAudioProcessor;

class ChorusView : public juce::Component, public juce::Timer {
public:
    ChorusView(VitalChorusAudioProcessor& p)
        : p_(p) {
        startTimerHz(30);
    }
    void paint(juce::Graphics& g) override;
private:
    VitalChorusAudioProcessor& p_;
    void timerCallback() override {
        repaint();
    }
};

class FilterView : public juce::Component {
public:
    FilterView(VitalChorusAudioProcessor& p)
        : p_(p) {}
    void paint(juce::Graphics& g) override;
private:
    VitalChorusAudioProcessor& p_;
};

// ---------------------------------------- editor ----------------------------------------
class VitalChorusAudioProcessorEditor final : public juce::AudioProcessorEditor {
public:
    explicit VitalChorusAudioProcessorEditor (VitalChorusAudioProcessor&);
    ~VitalChorusAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    VitalChorusAudioProcessor& p_;

    ui::Dial freq_{"freq"};
    ui::Dial tempo_{"tempo"};
    ui::Dial depth_{"depth"};
    ui::Dial delay1_{"delay1"};
    ui::Dial delay2_{"delay2"};
    ui::Dial feedback_{"feedback"};
    ui::Dial mix_{"mix"};
    ui::Dial cutoff_{"cutoff"};
    ui::Dial spread_{"spread"};
    ui::Dial num_voices_{"voices"};

    std::unique_ptr<juce::ParameterAttachment> sync_type_attach_;

    ChorusView chorus_view_;
    FilterView filter_view_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VitalChorusAudioProcessorEditor)
};

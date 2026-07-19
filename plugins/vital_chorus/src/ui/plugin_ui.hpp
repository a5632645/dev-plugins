#pragma once

#include <pluginshared/component.hpp>
#include <pluginshared/preset_panel.hpp>

#include "chorus_view.hpp"
#include "filter_view.hpp"

class VitalChorusAudioProcessor;

class PluginUi : public juce::Component {
public:
    static constexpr int kWidth = 530;
    static constexpr int kHeight = 200;

    explicit PluginUi(VitalChorusAudioProcessor& p);
    ~PluginUi() override;

    void resized() override;

private:
    pluginshared::PresetPanel preset_;
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
};

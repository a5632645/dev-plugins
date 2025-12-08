#pragma once
#include <unordered_map>
#include <juce_gui_basics/juce_gui_basics.h>
#include "chorus.hpp"
#include "delay.hpp"
#include "distortion.hpp"
#include "reverb.hpp"
#include "phaser.hpp"

class AnalogSynthAudioProcessor;

namespace analogsynth {
class Synth;

class FxChainGui : public juce::Component, private juce::Timer {
public:
    FxChainGui(Synth& synth, juce::AudioProcessor& processor);
    void resized() override;
private:
    void timerCallback() override;
    void RebuildInterface();
    void mouseDown(juce::MouseEvent const& e) override;
    void mouseDrag(juce::MouseEvent const& e) override;
    void mouseUp(juce::MouseEvent const& e) override;
    int getTargetIndexForUnevenHeights(int dragged_centre_y, juce::Component* dragged_component) const;

    Synth& synth_;
    juce::AudioProcessor& processor_;
    ChorusGui chorus_;
    DelayGui delay_;
    DistortionGui distortion_;
    ReverbGui reverb_;
    PhaserGui phaser;
    std::vector<juce::Component*> fx_sections_;
    std::unordered_map<juce::Component*, int> fx_height_hash_;
    std::unordered_map<juce::String, juce::Component*> fx_ptr_hash_;
    juce::ComponentDragger component_dragger_;
};
}
#include "PluginEditor.h"

#include "PluginProcessor.h"

class ControlGUI : public juce::Component {
public:
    ControlGUI(AudioPluginAudioProcessor& p, size_t i)
        : p_(p)
        , idx_(i)
    {
        a_.setRange(-0.9f, 0.9f);
        a_.onValueChange = [this] {
            p_.dsp_.apfs[idx_].SetAlpha(a_.getValue());
        };
        a_.setValue(0.0f);
        a_.setBounds(0, 0, 400, 25);
        a_.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
        addAndMakeVisible(a_);
        
        n_.setRange(1, 32767, 1);
        n_.onValueChange = [this] {
            p_.dsp_.apfs[idx_].SetNLatch(n_.getValue());
        };
        n_.setValue(1);
        n_.setBounds(0, 25, 400, 25);
        n_.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
        addAndMakeVisible(n_);
    }

    void paint(juce::Graphics& g) override {
        g.setColour(juce::Colours::white);
        g.drawRect(getLocalBounds());
    }

    juce::Slider n_;
    juce::Slider a_;

    AudioPluginAudioProcessor& p_;
    size_t idx_{};
};

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
    , tooltip_window_(this, 500)
{
    // setLookAndFeel(&look_);
    // tooltip_window_.setLookAndFeel(&look_);
    auto& apvsts = *p.value_tree_;

    button_add_.setButtonText("add");
    addAndMakeVisible(button_add_);
    button_add_.onClick = [this] {
        juce::ScopedLock _{processorRef.getCallbackLock()};

        auto& v = processorRef.dsp_.apfs.emplace_back();
        v.Init(32768);
        auto& gui = controls_.emplace_back(std::make_unique<ControlGUI>(processorRef, controls_.size()));
        gui->setBounds(0, 30 + (controls_.size() - 1) * 50, getWidth(), 50);
        addAndMakeVisible(*gui);
    };

    setSize (400, 800);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor() {
    stopTimer();
    setLookAndFeel(nullptr);
    tooltip_window_.setLookAndFeel(nullptr);
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g) {
    g.fillAll(findColour(juce::ResizableWindow::backgroundColourId));
}

void AudioPluginAudioProcessorEditor::resized() {
    auto b = getLocalBounds();
    button_add_.setBounds(b.removeFromTop(30));
}

void AudioPluginAudioProcessorEditor::timerCallback() {
}


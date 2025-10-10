#include "PluginEditor.h"
#include "PluginProcessor.h"

void ChorusView::paint(juce::Graphics& g) {
    g.fillAll(ui::black_bg);
    auto copy = p_.dsp_.delay_ms_;
    auto num_voices = static_cast<size_t>(p_.param_num_voices_->get());
    float const ytop = 0.25f * getHeight();
    float const ybottom = 0.75f * getHeight();
    g.setColour(ui::line_fore);
    for (size_t i = 0; i < num_voices / SimdType::kSize; ++i) {
        SimdType norm = copy[i] / SimdType::FromSingle(VitalChorus::kMaxDelayMs);
        SimdType x = norm * SimdType::FromSingle(getWidth());
        for (size_t j = 0; j < SimdType::kSize; ++j) {
            g.drawVerticalLine(x.x[j], ytop, ybottom);
        }
    }
}

void FilterView::paint(juce::Graphics& g) {
    g.fillAll(ui::black_bg);
}


// ---------------------------------------- editor ----------------------------------------
VitalChorusAudioProcessorEditor::VitalChorusAudioProcessorEditor (VitalChorusAudioProcessor& p)
    : AudioProcessorEditor (&p)
    , p_(p)
    , chorus_view_(p)
    , filter_view_(p)
{
    auto& apvts = *p.value_tree_;

    freq_.BindParam(apvts, "freq");
    addAndMakeVisible(freq_);
    depth_.BindParam(apvts, "depth");
    addAndMakeVisible(depth_);
    delay1_.BindParam(apvts, "delay1");
    addAndMakeVisible(delay1_);
    delay2_.BindParam(apvts, "delay2");
    addAndMakeVisible(delay2_);
    feedback_.BindParam(apvts, "feedback");
    addAndMakeVisible(feedback_);
    mix_.BindParam(apvts, "mix");
    addAndMakeVisible(mix_);
    cutoff_.BindParam(apvts, "cutoff");
    addAndMakeVisible(cutoff_);
    spread_.BindParam(apvts, "spread");
    addAndMakeVisible(spread_);
    num_voices_.BindParam(apvts, "num_voices");
    addAndMakeVisible(num_voices_);
    bypass_.BindParam(apvts, "bypass");
    addAndMakeVisible(bypass_);

    addAndMakeVisible(chorus_view_);
    addAndMakeVisible(filter_view_);

    setSize(530, 150);
    setResizable(true, true);
}

VitalChorusAudioProcessorEditor::~VitalChorusAudioProcessorEditor() {
}

//==============================================================================
void VitalChorusAudioProcessorEditor::paint (juce::Graphics& g) {
    g.fillAll(ui::green_bg);
}

void VitalChorusAudioProcessorEditor::resized() {
    auto b = getLocalBounds();

    auto w = b.getWidth() / 3;
    {
        auto left = b.removeFromLeft(w);
        auto top = left.removeFromTop(left.getHeight() / 2);
        num_voices_.setBounds(top.removeFromLeft(top.getWidth() / 2));
        freq_.setBounds(top);
        auto bottom = left;
        auto w = bottom.getWidth() / 3;
        depth_.setBounds(bottom.removeFromLeft(w));
        delay1_.setBounds(bottom.removeFromLeft(w));
        delay2_.setBounds(bottom);
    }
    {
        auto center = b.removeFromLeft(w);
        chorus_view_.setBounds(center.removeFromTop(center.getHeight() / 2).reduced(2, 2));
        filter_view_.setBounds(center.reduced(2, 2));
    }
    {
        auto right = b;
        auto top = right.removeFromTop(right.getHeight() / 2);
        feedback_.setBounds(top.removeFromLeft(top.getWidth() / 2));
        mix_.setBounds(top);
        auto bottom = right;
        cutoff_.setBounds(bottom.removeFromLeft(bottom.getWidth() / 2));
        spread_.setBounds(bottom);
    }
}

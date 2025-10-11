#include "PluginEditor.h"
#include "PluginProcessor.h"

#include "qwqdsp/convert.hpp"

void ChorusView::paint(juce::Graphics& g) {
    g.fillAll(ui::black_bg);
    auto copy = p_.dsp_.delay_ms_;
    auto num_voices = static_cast<size_t>(p_.param_num_voices_->get());
    float const ytop = 0.25f * getHeight();
    float const ybottom = 0.75f * getHeight();
    for (size_t i = 0; i < num_voices / SimdType::kSize; ++i) {
        SimdType norm = copy[i] / SimdType::FromSingle(VitalChorus::kMaxDelayMs);
        SimdType x = norm * SimdType::FromSingle(getWidth());
        // left
        g.setColour(ui::line_fore);
        for (size_t j = 0; j < SimdType::kSize; j += 2) {
            g.drawVerticalLine(x.x[j], ytop, ybottom);
        }
        // right
        g.setColour(ui::active_bg);
        for (size_t j = 1; j < SimdType::kSize; j += 2) {
            g.drawVerticalLine(x.x[j], ytop, ybottom);
        }
    }
}

void FilterView::paint(juce::Graphics& g) {
    g.fillAll(ui::black_bg);
    
    auto[lp, hp] = p_.dsp_.GetFilterResponceCalc();
    auto eval_y = [&lp, &hp, h = static_cast<float>(getHeight()), fs = static_cast<float>(p_.getSampleRate())](float norm_w) {
        static float const pitch_begin = qwqdsp::convert::Freq2Pitch(20.0f);
        static float const pitch_end = qwqdsp::convert::Freq2Pitch(20000.0f);
        float const pitch = pitch_begin + norm_w * (pitch_end - pitch_begin);
        float const w = qwqdsp::convert::Pitch2Freq(pitch) / fs * std::numbers::pi_v<float> * 2.0f;
        float gg = std::abs(lp(w) * hp(w));
        gg = qwqdsp::convert::Gain2Db<-24.0f>(gg);
        gg = std::min(gg, 0.0f);
        return juce::jmap(gg, -24.0f, 12.0f, h, 0.0f);
    };

    g.setColour(ui::line_fore);
    int const w = getWidth();
    juce::Point<float> last{0, eval_y(0.0f)};
    for (int i = 1; i < w; ++i) {
        juce::Point<float> curr{static_cast<float>(i), eval_y(static_cast<float>(i) / static_cast<float>(w))};
        g.drawLine(juce::Line<float>{last, curr});
        last = curr;
    }
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
    cutoff_.slider.onValueChange = [this] {
        filter_view_.repaint();
    };
    addAndMakeVisible(cutoff_);
    spread_.slider.onValueChange = [this] {
        filter_view_.repaint();
    };
    spread_.BindParam(apvts, "spread");
    addAndMakeVisible(spread_);
    num_voices_.BindParam(apvts, "num_voices");
    addAndMakeVisible(num_voices_);
    bypass_.BindParam(apvts, "bypass");
    addAndMakeVisible(bypass_);

    addAndMakeVisible(chorus_view_);
    addAndMakeVisible(filter_view_);

    setSize(530, 150);
    setResizeLimits(530, 150, 9999, 9999);
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

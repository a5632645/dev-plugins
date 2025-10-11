#include "PluginEditor.h"
#include "PluginProcessor.h"

#include "qwqdsp/convert.hpp"

void ChorusView::paint(juce::Graphics& g) {
    g.fillAll(ui::black_bg);
    auto copy = p_.dsp_.delay_ms_;
    auto num_voices = static_cast<size_t>(p_.param_num_voices_->get());
    float const ytop = 0.25f * static_cast<float>(getHeight());
    float const ybottom = 0.75f * static_cast<float>(getHeight());
    for (size_t i = 0; i < num_voices / SimdType::kSize; ++i) {
        SimdType norm = copy[i] / SimdType::FromSingle(VitalChorus::kMaxDelayMs);
        SimdType x = norm * SimdType::FromSingle(static_cast<float>(getWidth()));
        // left
        g.setColour(ui::line_fore);
        for (size_t j = 0; j < SimdType::kSize; j += 2) {
            g.drawVerticalLine(static_cast<int>(x.x[j]), ytop, ybottom);
        }
        // right
        g.setColour(ui::active_bg);
        for (size_t j = 1; j < SimdType::kSize; j += 2) {
            g.drawVerticalLine(static_cast<int>(x.x[j]), ytop, ybottom);
        }
    }
}

void FilterView::paint(juce::Graphics& g) {
    g.fillAll(ui::black_bg);
    
    auto[lp, hp] = p_.dsp_.GetFilterResponceCalc();
    auto eval_y = [&lp, &hp, h = static_cast<float>(getHeight()), fs = static_cast<float>(p_.getSampleRate())](float norm_w) {
        constexpr float pitch_begin = 8;
        constexpr float pitch_end = 136;
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

    auto sync_type_changed = [this](float){
        LFOTempoType type = static_cast<LFOTempoType>(p_.param_sync_type_->get());
        if (type == LFOTempoType::Free) {
            freq_.setVisible(true);
            tempo_.setVisible(false);
        }
        else {
            freq_.setVisible(false);
            tempo_.setVisible(true);
        }

        if (type == LFOTempoType::Sync) {
            tempo_.label.setText("tempo", juce::dontSendNotification);
        }
        else if (type == LFOTempoType::SyncDot) {
            tempo_.label.setText("tempo dot", juce::dontSendNotification);
        }
        else if (type == LFOTempoType::SyncTri) {
            tempo_.label.setText("tempo triplets", juce::dontSendNotification);
        }
    };
    sync_type_attach_ = std::make_unique<juce::ParameterAttachment>(
        *p.param_sync_type_,
        sync_type_changed
    );
    freq_.BindParam(apvts, "freq");
    freq_.OnMenuShowup() = [this](juce::PopupMenu& menu) {
        menu.addSeparator();
        menu.addItem("tempo", [&attach = sync_type_attach_]{
            attach->setValueAsCompleteGesture(static_cast<float>(LFOTempoType::Sync));
        });
        menu.addItem("dot", [&attach = sync_type_attach_]{
            attach->setValueAsCompleteGesture(static_cast<float>(LFOTempoType::SyncDot));
        });
        menu.addItem("triplet", [&attach = sync_type_attach_]{
            attach->setValueAsCompleteGesture(static_cast<float>(LFOTempoType::SyncTri));
        });
    };
    addChildComponent(freq_);
    tempo_.BindParam(apvts, "tempo");
    tempo_.OnMenuShowup() = [this](juce::PopupMenu& menu) {
        menu.addSeparator();
        menu.addItem("hz", [&attach = sync_type_attach_]{
            attach->setValueAsCompleteGesture(static_cast<float>(LFOTempoType::Free));
        });
        menu.addItem("dot", [&attach = sync_type_attach_]{
            attach->setValueAsCompleteGesture(static_cast<float>(LFOTempoType::SyncDot));
        });
        menu.addItem("triplet", [&attach = sync_type_attach_]{
            attach->setValueAsCompleteGesture(static_cast<float>(LFOTempoType::SyncTri));
        });
    };
    addChildComponent(tempo_);
    // force update gui
    sync_type_changed(0);

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
        tempo_.setBounds(top);
        auto bottom = left;
        auto w2 = bottom.getWidth() / 3;
        depth_.setBounds(bottom.removeFromLeft(w2).reduced(1, 0));
        delay1_.setBounds(bottom.removeFromLeft(w2).reduced(1, 0));
        delay2_.setBounds(bottom.reduced(1, 0));
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

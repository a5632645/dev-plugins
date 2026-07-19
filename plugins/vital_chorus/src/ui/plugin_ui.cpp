#include "plugin_ui.hpp"

#include "../PluginProcessor.h"

//==============================================================================
// PluginUi
//==============================================================================
PluginUi::PluginUi(VitalChorusAudioProcessor& p)
    : preset_(*p.preset_manager_)
    , p_(p)
    , chorus_view_(p)
    , filter_view_(p)
{
    addAndMakeVisible(preset_);

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

    setSize(kWidth, kHeight);
}

PluginUi::~PluginUi() = default;

//==============================================================================
void PluginUi::resized() {
    auto b = getLocalBounds();
    preset_.setBounds(b.removeFromTop(std::max(30, b.proportionOfHeight(0.1f))));

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

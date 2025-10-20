#pragma once
#include "component.hpp"
#include "bpm_sync_lfo.hpp"

namespace ui {
class BpmSyncDial : public juce::Component {
public:
    BpmSyncDial(juce::StringRef freq_name, juce::StringRef tempo_name)
        : freq_dial_(freq_name)
        , tempo_dial_(tempo_name) {
        addAndMakeVisible(freq_dial_);
        addChildComponent(tempo_dial_);
    }

    template<bool NegPos>
    void BindParam(pluginshared::BpmSyncLFO<NegPos>& lfo_state) {
        using LFOTempoType = pluginshared::BpmSyncLFO<NegPos>::LFOTempoType;
        auto sync_type_changed = [this, &lfo_state](float){
            LFOTempoType type = static_cast<LFOTempoType>(lfo_state.param_lfo_tempo_type_->get());
            if (type == LFOTempoType::Free) {
                freq_dial_.setVisible(true);
                tempo_dial_.setVisible(false);
            }
            else {
                freq_dial_.setVisible(false);
                tempo_dial_.setVisible(true);
            }

            if (type == LFOTempoType::Sync) {
                tempo_dial_.label.setText("tempo", juce::dontSendNotification);
            }
            else if (type == LFOTempoType::SyncDot) {
                tempo_dial_.label.setText("tempo dot", juce::dontSendNotification);
            }
            else if (type == LFOTempoType::SyncTri) {
                tempo_dial_.label.setText("tempo triplets", juce::dontSendNotification);
            }
        };
        sync_type_attach_ = std::make_unique<juce::ParameterAttachment>(
            *lfo_state.param_lfo_tempo_type_,
            sync_type_changed
        );

        freq_dial_.BindParam(lfo_state.param_lfo_hz_);
        freq_dial_.OnMenuShowup() = [this](juce::PopupMenu& menu) {
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
        tempo_dial_.BindParam(lfo_state.param_tempo_speed_);
        tempo_dial_.OnMenuShowup() = [this](juce::PopupMenu& menu) {
            menu.addSeparator();
            menu.addItem("hz", [&attach = sync_type_attach_]{
                attach->setValueAsCompleteGesture(static_cast<float>(LFOTempoType::Free));
            });
            menu.addItem("tempo", [&attach = sync_type_attach_] {
                attach->setValueAsCompleteGesture(static_cast<float>(LFOTempoType::Sync));
            });
            menu.addItem("dot", [&attach = sync_type_attach_]{
                attach->setValueAsCompleteGesture(static_cast<float>(LFOTempoType::SyncDot));
            });
            menu.addItem("triplet", [&attach = sync_type_attach_]{
                attach->setValueAsCompleteGesture(static_cast<float>(LFOTempoType::SyncTri));
            });
        };
        // force update gui
        sync_type_changed(0);
    }

    void resized() override {
        auto b = getLocalBounds();
        freq_dial_.setBounds(b);
        tempo_dial_.setBounds(b);
    }

    ui::Dial freq_dial_;
    ui::Dial tempo_dial_;
private:
    std::unique_ptr<juce::ParameterAttachment> sync_type_attach_;
};
}
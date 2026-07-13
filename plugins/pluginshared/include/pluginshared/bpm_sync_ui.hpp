#pragma once
#include "bpm_sync_lfo.hpp"
#include "component.hpp"

namespace ui {
class BpmSyncDial : public juce::Component {
public:
    BpmSyncDial(juce::StringRef freq_name)
        : freq_dial_(freq_name) {
        addAndMakeVisible(freq_dial_);
    }

    void BindParam(pluginshared::BpmSyncLFO& lfo_state) {
        jassert(lfo_state.param_freq != nullptr);
        jassert(lfo_state.param_type != nullptr);

        sync_type_attach_ = nullptr;
        sync_type_attach_ =
            std::make_unique<juce::ParameterAttachment>(*lfo_state.param_type, [this, &lfo_state](float) {
                freq_dial_.slider.updateText();
                freq_dial_.slider.setDoubleClickReturnValue(true, lfo_state.GetFreqAttribute().tempo_sync
                                                                      ? lfo_state.GetDefaultTempoVal01()
                                                                      : lfo_state.GetDefaultHzVal01());
            });

        freq_dial_.BindParam(lfo_state.param_freq);
        freq_dial_.OnMenuShowup() = [this, &lfo_state](juce::PopupMenu& menu) {
            pluginshared::BpmSyncLFO::FreqAttrubute attr = lfo_state.GetFreqAttribute();
            menu.addSeparator();
            menu.addItem("tempo sync", true, attr.tempo_sync, [this, attr]() mutable {
                attr.tempo_sync = !attr.tempo_sync;
                sync_type_attach_->setValueAsCompleteGesture(static_cast<float>(std::bit_cast<int>(attr)));
            });
            menu.addItem("ppq sync", true, attr.ppq_sync, [this, attr]() mutable {
                attr.ppq_sync = !attr.ppq_sync;
                sync_type_attach_->setValueAsCompleteGesture(static_cast<float>(std::bit_cast<int>(attr)));
            });
            menu.addItem("tempo snap", true, attr.tempo_snap, [this, attr]() mutable {
                attr.tempo_snap = !attr.tempo_snap;
                sync_type_attach_->setValueAsCompleteGesture(static_cast<float>(std::bit_cast<int>(attr)));
            });

            menu.addSeparator();
            menu.addItem("reset phase", [this, &lfo_state]() {
                lfo_state.TryResetPhase(reset_phase_);
            });
        };

        freq_dial_.slider.setDoubleClickReturnValue(true, lfo_state.GetFreqAttribute().tempo_sync
                                                              ? lfo_state.GetDefaultTempoVal01()
                                                              : lfo_state.GetDefaultHzVal01());
    }

    void resized() override {
        auto b = getLocalBounds();
        freq_dial_.setBounds(b);
    }

    void SetResetPhase(float phase) {
        reset_phase_ = phase;
    }

    ui::Dial freq_dial_;
private:
    std::unique_ptr<juce::ParameterAttachment> sync_type_attach_;
    float reset_phase_{};
};
} // namespace ui

#include "plugin_ui.hpp"

#include "PluginProcessor.h"


namespace debugger {

PluginUi::PluginUi(EmptyAudioProcessor& p)
    : preset_(*p.preset_manager_) {
    addAndMakeVisible(preset_);
    pitch_.BindParam(p.param_pitch_shift.ptr_);
    addAndMakeVisible(pitch_);
    size_.BindParam(p.param_grain_size.ptr_);
    addAndMakeVisible(size_);
}

void PluginUi::resized() {
    auto b = getLocalBounds();
    preset_.setBounds(b.removeFromTop(30));

    auto box = b.removeFromTop(65);
    pitch_.setBounds(box.removeFromLeft(50));
    size_.setBounds(box.removeFromLeft(50));
}

}  // namespace debugger

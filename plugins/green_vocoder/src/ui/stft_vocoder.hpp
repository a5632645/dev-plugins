#pragma once
#include <pluginshared/component.hpp>

class AudioPluginAudioProcessor;

namespace green_vocoder::ui {

class STFTVocoder : public juce::Component {
public:
    STFTVocoder(AudioPluginAudioProcessor& processor);
    void resized() override;
private:
    void OnModeChanged();

    AudioPluginAudioProcessor& processor_;
    ::ui::Dial release_{"release"};
    ::ui::Dial attack_{"attack"};
    ::ui::FlatCombobox size_;
    ::ui::FlatCombobox mode_;

    ::ui::Dial blend_{"noisy"};
    ::ui::Dial bandwidth_{"smear"};
    ::ui::Dial detail_{"detail"};
    ::ui::Switch smooth_type_{"ERB", "OCT"};
    ::ui::Dial smooth_{"smooth"};
    ::ui::Dial welch_{"avg"};
    ::ui::Dial floor_{"floor"};
    ::ui::Dial morph_{"morph"};
    ::ui::Dial mfcc_size_{"bands"};
    ::ui::Switch direction_{"A→B", "B→A"};
    ::ui::Switch wiener_glitch_{"glitch"};
    ::ui::Switch wiener_ab_{"A→B", "B→A"};
};

} // namespace green_vocoder::ui

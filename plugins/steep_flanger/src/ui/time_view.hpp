#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "global.hpp"
#include "pluginshared/component.hpp"

class SteepFlangerAudioProcessor;

class TimeView : public juce::Component {
public:
    TimeView(SteepFlangerAudioProcessor& p)
        : p_(p) {}

    void paint(juce::Graphics& g) override;

    void mouseDown(const juce::MouseEvent& e) override {
        if (!display_waveform_) return;
        mouseDrag(e);
    }

    void mouseDrag(const juce::MouseEvent& e) override;

    void mouseUp(const juce::MouseEvent& e) override;

    // -------------------- time coeffs --------------------
    void SetDisplayWaveform(bool display) {
        display_waveform_ = display;
        repaint();
    }

    void SendCoeffs();

    void CopyCoeffesToCustom();

    void ClearCustomCoeffs();

    void UpdateGui();
private:
    void RepaintTimeAndSpectralView();

    SteepFlangerAudioProcessor& p_;
    std::array<float, global::kMaxCoeffLen + 1> coeff_buffer_{};
    bool display_waveform_{true};

    friend class SpectralView;
};

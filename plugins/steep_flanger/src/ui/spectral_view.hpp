#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "pluginshared/component.hpp"
#include "qwqdsp/spectral/real_fft.hpp"

class TimeView;
class SteepFlangerAudioProcessor;

class SpectralView : public juce::Component {
public:
    static constexpr float kDbCeil = 20.0f;
    static constexpr float kDbFloor = -100.0f;
    static constexpr float kDbStep = 20.0f;

    SpectralView(TimeView& time, SteepFlangerAudioProcessor& processor)
        : time_(time)
        , p_(processor) {
        fft_.Init(kGainFFTSize);
    }

    void paint(juce::Graphics& g) override;

    void UpdateGui();

    void mouseDown(const juce::MouseEvent& e) override;

    void mouseDrag(const juce::MouseEvent& e) override;

    void mouseUp(const juce::MouseEvent& e) override;

    void SetIir(bool iir) {
        iir_ = iir;
        repaint();
    }

    void DrawIirResponce() {
        repaint();
    }
private:
    void DrawIir(juce::Graphics& g);

    static constexpr size_t kGainFFTSize = 1024;
    static constexpr size_t kGainNumBins = qwqdsp_spectral::RealFFT::NumBins(kGainFFTSize);

    TimeView& time_;
    SteepFlangerAudioProcessor& p_;
    std::array<float, kGainNumBins> gains_{};
    qwqdsp_spectral::RealFFT fft_;
    bool iir_{false};
};

#pragma once
#include "pluginshared/component.hpp"
#include "pluginshared/preset_panel.hpp"

#include "qwqdsp/spectral/real_fft.hpp"
#include "shared.hpp"

class DeepPhaserAudioProcessor;

// ---------------------------------------- time prev ----------------------------------------
class TimeView : public juce::Component {
public:
    TimeView(DeepPhaserAudioProcessor& p)
        : p_(p)
    {
        addAndMakeVisible(title_);
        reload_.onClick = [this] {
            SendCoeffs();
        };
        reload_.setButtonText("reload");
        addAndMakeVisible(reload_);

        copy_.onClick = [this] {
            CopyCoeffesToCustom();
        };
        copy_.setButtonText("copy");
        addAndMakeVisible(copy_);

        clear_.onClick = [this] {
            ClearCustomCoeffs();
        };
        clear_.setButtonText("clear");
        addAndMakeVisible(clear_);

        display_custom_.setToggleState(true, juce::dontSendNotification);
        addAndMakeVisible(display_custom_);
        display_custom_.onStateChange = [this] {
            RepaintTimeAndSpectralView();
        };
    }

    void paint(juce::Graphics& g) override;

    void mouseDown(const juce::MouseEvent& e) override {
        mouseDrag(e);
    }

    void mouseDrag(const juce::MouseEvent& e) override;

    void mouseUp(const juce::MouseEvent& e) override;

    void SendCoeffs();

    void CopyCoeffesToCustom();

    void ClearCustomCoeffs();

    void resized() override {
        auto b = getLocalBounds();
        auto top = b.removeFromTop(title_.getFont().getHeight() * 1.5f);
        reload_.setBounds(top.removeFromRight(60).reduced(1, 1));
        copy_.setBounds(top.removeFromRight(50).reduced(1, 1));
        clear_.setBounds(top.removeFromRight(50).reduced(1, 1));
        display_custom_.setBounds(top.removeFromRight(70).reduced(1, 1));
        title_.setBounds(top);
    }

    void UpdateGui();

private:
    void RepaintTimeAndSpectralView();

    DeepPhaserAudioProcessor& p_;
    juce::Label title_{"", "Time view"};
    ui::FlatButton reload_;
    ui::FlatButton copy_;
    ui::FlatButton clear_;
    ui::Switch display_custom_{"show ctm"};
    std::array<float, kMaxCoeffLen + 1> coeff_buffer_{};

    friend class SpectralView;
};

class SpectralView : public juce::Component {
public:
    SpectralView(TimeView& time)
        : time_(time)
    {
        addAndMakeVisible(title_);
        fft_.Init(kGainFFTSize);
    }

    void paint(juce::Graphics& g) override;

    void resized() override {
        auto b = getLocalBounds();
        title_.setBounds(b.removeFromTop(title_.getFont().getHeight()));
    }

    void UpdateGui();

    void mouseDown(const juce::MouseEvent& e) override {
        mouseDrag(e);
    }

    void mouseDrag(const juce::MouseEvent& e) override;

    void mouseUp(const juce::MouseEvent& e) override;

private:
    static constexpr size_t kGainFFTSize = 1024;
    static constexpr size_t kGainNumBins = qwqdsp::spectral::RealFFT::NumBins(kGainFFTSize);

    TimeView& time_;
    juce::Label title_{"", "Responce"};
    std::array<float, kGainNumBins> gains_{};
    float max_db_{};
    float min_db_{};
    qwqdsp::spectral::RealFFT fft_;
};

// ---------------------------------------- editor ----------------------------------------

//==============================================================================
class DeepPhaserAudioProcessorEditor final 
    : public juce::AudioProcessorEditor
    , public juce::Timer
{
public:
    explicit DeepPhaserAudioProcessorEditor (DeepPhaserAudioProcessor&);
    ~DeepPhaserAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    void UpdateGui() {
        timeview_.UpdateGui();
        spectralview_.UpdateGui();
    }

    void UpdateGuiFromTimeView() {
        spectralview_.UpdateGui();
    }

    void timerCallback() override;
private:
    DeepPhaserAudioProcessor& p_;
    pluginshared::PresetPanel preset_panel_;

    juce::Label allpass_title_{"basic", "basic"};
    ui::Dial state_{"state"};
    ui::Dial allpass_blend_{"blend"};
    ui::Dial fb_value_{"feedback"};
    ui::Dial fb_damp_{"damp"};
    ui::FlatButton panic_;

    juce::Label fir_title_{"fir", "fir"};
    ui::Dial cutoff_{"cutoff"};
    ui::Dial coeff_len_{"steep"};
    ui::Dial side_lobe_{"side_lobe"};
    ui::Switch minum_phase_{"minum_phase"};
    ui::Switch highpass_{"highpass"};
    ui::Switch custom_{"custom"};

    juce::Label barber_title_{"barberpole", "barberpole"};
    ui::Switch barber_enable_{"barberpole"};
    ui::Dial barber_phase_{"phase"};
    ui::Dial barber_speed_{"speed"};
    ui::FlatButton barber_reset_phase_;

    TimeView timeview_;
    SpectralView spectralview_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DeepPhaserAudioProcessorEditor)
};

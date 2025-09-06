#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "juce_core/juce_core.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include "juce_events/juce_events.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "param_ids.hpp"
#include "tooltips.hpp"
#include "widget/channel_vocoder.hpp"
#include "widget/ensemble.hpp"
#include "widget/performance.hpp"
#include "widget/stft_vocoder.hpp"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
    , tooltip_window_(this, 500)
    , main_gain_(p.main_gain_)
    , side_gain_(p.side_gain_)
    , output_gain_(p.output_gain_)
    , stft_vocoder_(p)
    , burg_lpc_(p)
    , rls_lpc_(p)
    , channel_vocoder_(p)
    , ensemble_(p)
    , tracking_(p)
#ifdef __VOCODER_ENABLE_PERFORMANCE_DEBUG
    , performance_(p.perf_)
#endif
{
    setLookAndFeel(&myLookAndFeel_);
    tooltip_window_.setLookAndFeel(&myLookAndFeel_);
    
    auto& apvts = *p.value_tree_;

    addAndMakeVisible(filter_);
    em_pitch_.BindParameter(apvts, id::kEmphasisPitch);
    addAndMakeVisible(em_pitch_);
    em_gain_.BindParameter(apvts, id::kEmphasisGain);
    addAndMakeVisible(em_gain_);
    em_s_.BindParameter(apvts, id::kEmphasisS);
    addAndMakeVisible(em_s_);

    addAndMakeVisible(shifter_);
    shift_enable_.BindParameter(apvts, id::kEnableShifter);
    shift_enable_.onStateChange = [this] {
        shift_pitch_.setEnabled(shift_enable_.getToggleState());
    };
    addAndMakeVisible(shift_enable_);
    shift_pitch_.BindParameter(apvts, id::kShiftPitch);
    shift_pitch_.setEnabled(shift_enable_.getToggleState());
    addAndMakeVisible(shift_pitch_);

    main_gain_.gain_slide_.BindParameter(apvts, id::kMainGain);
    addAndMakeVisible(main_gain_);
    side_gain_.gain_slide_.BindParameter(apvts, id::kSideGain);
    addAndMakeVisible(side_gain_);
    output_gain_.gain_slide_.BindParameter(apvts, id::kOutputgain);
    addAndMakeVisible(output_gain_);
    main_channel_selector_.BindParameter(apvts, id::kMainChannelConfig);
    addAndMakeVisible(main_channel_selector_);
    side_channel_selector_.BindParameter(apvts, id::kSideChannelConfig);
    addAndMakeVisible(side_channel_selector_);

    vocoder_type_.combobox_.addListener(this);
    vocoder_type_.BindParam(apvts, id::kVocoderType);
    addAndMakeVisible(vocoder_type_);

    language_box_.addItem("English", 1);
    language_box_.addItem(juce::String::fromUTF8("中文"), 2);
    language_box_.addListener(this);
    language_box_.setSelectedItemIndex(0);
    addAndMakeVisible(language_box_);

    addChildComponent(stft_vocoder_);
    addChildComponent(rls_lpc_);
    addChildComponent(burg_lpc_);
    addChildComponent(channel_vocoder_);

    addAndMakeVisible(ensemble_);
    addAndMakeVisible(tracking_);
#ifdef __VOCODER_ENABLE_PERFORMANCE_DEBUG
    addAndMakeVisible(performance_);
#endif

    setSize (575, 550);
    startTimerHz(30);
    OnLanguageChanged(tooltips_);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor() {
    stopTimer();
    setLookAndFeel(nullptr);
    tooltip_window_.setLookAndFeel(nullptr);
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g) {
    g.fillAll(findColour(juce::ResizableWindow::backgroundColourId));
}

void AudioPluginAudioProcessorEditor::OnLanguageChanged(tooltip::Tooltips& tooltips) {
    filter_.setText(tooltips.Label(id::kFilterTitle), juce::dontSendNotification);
    main_channel_selector_.SetLabelName(tooltips.Label(id::kMainChannelConfig));
    side_channel_selector_.SetLabelName(tooltips.Label(id::kSideChannelConfig));
    em_pitch_.OnLanguageChanged(tooltips);
    em_gain_.OnLanguageChanged(tooltips);
    em_s_.OnLanguageChanged(tooltips);
    shift_pitch_.OnLanguageChanged(tooltips);
    vocoder_type_.OnLanguageChanged(tooltips);
    stft_vocoder_.OnLanguageChanged(tooltips);
    burg_lpc_.OnLanguageChanged(tooltips);
    rls_lpc_.OnLanguageChanged(tooltips);
    channel_vocoder_.OnLanguageChanged(tooltips);
    ensemble_.OnLanguageChanged(tooltips);
    main_gain_.gain_slide_.OnLanguageChanged(tooltips);
    side_gain_.gain_slide_.OnLanguageChanged(tooltips);
    output_gain_.gain_slide_.OnLanguageChanged(tooltips);
    tracking_.ChangeLang(tooltips);
}

void AudioPluginAudioProcessorEditor::resized() {
    auto b = getLocalBounds();
    {
        auto title_box = b.removeFromTop(20);
        filter_.setBounds(title_box);
        auto top = b.removeFromTop(100);
        em_pitch_.setBounds(top.removeFromLeft(50));
        em_gain_.setBounds(top.removeFromLeft(50));
        em_s_.setBounds(top.removeFromLeft(50));
        shift_pitch_.setBounds(top.removeFromLeft(50));
        {
            title_box.removeFromLeft(shift_pitch_.getX());
            shift_enable_.setBounds(title_box.removeFromLeft(25));
            shifter_.setBounds(title_box);
            language_box_.setBounds(title_box.removeFromRight(100));
        }
        main_gain_.setBounds(top.removeFromLeft(50 + 20));
        side_gain_.setBounds(top.removeFromLeft(50 + 20));
        output_gain_.setBounds(top.removeFromLeft(50 + 40));
        {
            auto half_top = top.removeFromTop(top.getHeight() / 2);
            main_channel_selector_.setBounds(half_top);
            side_channel_selector_.setBounds(top);
        }
    }
    {
        vocoder_type_.setBounds(b.removeFromTop(30));
    }
    {
        auto top = b.removeFromBottom(120);
        ensemble_.setBounds(top.removeFromLeft(250));
        top.removeFromLeft(10);
        tracking_.setBounds(top);
#ifdef __VOCODER_ENABLE_PERFORMANCE_DEBUG
        performance_.setBounds(ensemble_.getBounds().removeFromRight(300).removeFromBottom(50));
#endif
    }
    {
        burg_lpc_.setBounds(b);
        rls_lpc_.setBounds(b);
        stft_vocoder_.setBounds(b);
        channel_vocoder_.setBounds(b);
    }
}

void AudioPluginAudioProcessorEditor::timerCallback() {
    if (current_vocoder_widget_ != nullptr) {
        current_vocoder_widget_->repaint();
    }
    main_gain_.repaint();
    side_gain_.repaint();
    output_gain_.repaint();
#ifdef __VOCODER_ENABLE_PERFORMANCE_DEBUG
    performance_.Update();
#endif
}

void AudioPluginAudioProcessorEditor::comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) {
    if (comboBoxThatHasChanged == &vocoder_type_.combobox_) {
        if (current_vocoder_widget_ != nullptr) {
            current_vocoder_widget_->setVisible(false);
        }

        switch (vocoder_type_.combobox_.getSelectedItemIndex()) {
        case eVocoderType_ChannelVocoder:
            current_vocoder_widget_ = &channel_vocoder_;
            break;
        case eVocoderType_STFTVocoder:
            current_vocoder_widget_ = &stft_vocoder_;
            break;
        case eVocoderType_BurgLPC:
            current_vocoder_widget_ = &burg_lpc_;
            break;
        case eVocoderType_RLSLPC:
            current_vocoder_widget_ = &rls_lpc_;
            break;
        default:
            jassertfalse;
            break;
        }

        current_vocoder_widget_->setVisible(true);
    }
    else if (comboBoxThatHasChanged == &language_box_) {
        switch (language_box_.getSelectedItemIndex()) {
        case 0:
            tooltips_.MakeEnglishTooltips();
            break;
        case 1:
            tooltips_.MakeChineseTooltips();
            break;
        };
        OnLanguageChanged(tooltips_);
    }
}

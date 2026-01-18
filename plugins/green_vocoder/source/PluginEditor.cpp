#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "param_ids.hpp"

struct AudioPluginAudioProcessorEditor::PluginConfig {
    PluginConfig() {
        juce::PropertiesFile::Options options;
        options.applicationName = JucePlugin_Name;
        options.filenameSuffix = ".settings";
        options.folderName = JucePlugin_Manufacturer;
        options.storageFormat = juce::PropertiesFile::storeAsXML;

        config = std::make_unique<juce::PropertiesFile>(options);
    }

    std::unique_ptr<juce::PropertiesFile> config;
};

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor& p)
    : AudioProcessorEditor(&p)
    , ui_(p) {
    auto* props = plugin_config_->config.get();

    addAndMakeVisible(ui_);
    if (props != nullptr) {
        setSize(props->getIntValue("last_width", ui_.kWidth), props->getIntValue("last_height", ui_.kHeight));
    }
    else {
        setSize(ui_.kWidth, ui_.kHeight);
    }
    setResizable(true, true);
    getConstrainer()->setFixedAspectRatio(static_cast<float>(ui_.kWidth) / ui_.kHeight);
    setResizeLimits(ui_.kWidth, ui_.kHeight, 9999, 9999);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor() {}

//==============================================================================

void AudioPluginAudioProcessorEditor::resized() {
    float scaleX = static_cast<float>(getWidth()) / ui_.kWidth;
    float scaleY = static_cast<float>(getHeight()) / ui_.kHeight;
    ui_.setTransform(juce::AffineTransform::scale(scaleX, scaleY));
    ui_.setBounds(0, 0, ui_.kWidth, ui_.kHeight);

    if (auto* props = plugin_config_->config.get()) {
        if (getWidth() > 0 && getHeight() > 0) {
            props->setValue("last_width", getWidth());
            props->setValue("last_height", getHeight());
        }
    }
}

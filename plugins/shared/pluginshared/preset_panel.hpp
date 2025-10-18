#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "preset_manager.hpp"

namespace pluginshared {
class PresetPanel : public juce::Component, juce::Button::Listener, juce::ComboBox::Listener {
public:
    static constexpr int kButtonInfoDisplayTime = 5000;
    static constexpr auto kReleasePageURL = "https://github.com/a5632645/dev-plugins/releases";
    static constexpr auto kReleaseJsonFile = "https://raw.githubusercontent.com/a5632645/dev-plugins/refs/heads/main/release.json";

    PresetPanel(PresetManager& pm) 
        : presetManager(pm) {
        configureButton(saveButton, "Save");
        configureButton(deleteButton, "Delete");
        configureButton(previousPresetButton, "<");
        configureButton(nextPresetButton, ">");

        presetList.setTextWhenNothingSelected("default");
        addAndMakeVisible(presetList);
        presetList.addListener(this);

        loadPresetList();

        juce::String s;
        s << JucePlugin_Name << ' ' << JucePlugin_VersionString;
        version_label_.setText(s, juce::dontSendNotification);
        addAndMakeVisible(version_label_);

        check_update_.setButtonText("check update");
        addAndMakeVisible(check_update_);
        check_update_.addListener(this);
    }

    ~PresetPanel() override {
        saveButton.removeListener(this);
        deleteButton.removeListener(this);
        previousPresetButton.removeListener(this);
        nextPresetButton.removeListener(this);
        presetList.removeListener(this);
        if (update_thread_) {
            update_thread_->stopThread(100);
            update_thread_ = nullptr;
        }
    }

    void resized() override {
        auto container = getLocalBounds().reduced(4);

        auto version_bound = container.removeFromLeft(container.proportionOfWidth(0.3f));
        float const string_width =juce::TextLayout::getStringWidth(version_label_.getFont(), version_label_.getText());
        version_label_.setBounds(version_bound.removeFromLeft(static_cast<int>(string_width)));
        check_update_.setBounds(version_bound.reduced(4));

        auto const b = container;
        deleteButton.setBounds(container.removeFromRight(b.proportionOfWidth(0.15f)).reduced(4));
        saveButton.setBounds(container.removeFromRight(b.proportionOfWidth(0.15f)).reduced(4));
        previousPresetButton.setBounds(container.removeFromLeft(container.getHeight()).reduced(4));
        nextPresetButton.setBounds(container.removeFromRight(container.getHeight()).reduced(4));
        presetList.setBounds(container.reduced(4));
    }
private:
    void buttonClicked(juce::Button* button) override {
        if (button == &saveButton) {
            fileChooser = std::make_unique<juce::FileChooser>(
                "Please enter the name of the preset to save",
                PresetManager::defaultDirectory,
                "*." + PresetManager::extension
            );
            fileChooser->launchAsync(juce::FileBrowserComponent::saveMode, [&](const juce::FileChooser& chooser) {
                    const auto resultFile = chooser.getResult();
                    presetManager.savePreset(resultFile.getFileNameWithoutExtension());
                    loadPresetList();
                });
        }
        else if (button == &previousPresetButton) {
            const auto index = presetManager.loadPreviousPreset();
            presetList.setSelectedItemIndex(index, juce::dontSendNotification);
        }
        else if (button == &nextPresetButton) {
            const auto index = presetManager.loadNextPreset();
            presetList.setSelectedItemIndex(index, juce::dontSendNotification);
        }
        else if (button == &deleteButton) {
            if (presetList.getSelectedId() == 1) {
                return;
            }
            presetManager.deletePreset(presetManager.getCurrentPreset());
            loadPresetList();
        }
        else if (button == &check_update_) {
            if (have_new_version_) {
                juce::URL{kReleasePageURL}.launchInDefaultBrowser();
            }
            else {
                if (update_thread_) {
                    update_thread_->stopThread(100);
                    update_thread_ = nullptr;
                }
                update_thread_ = std::make_unique<UpdateThread>(*this);
                if (!update_thread_->startThread()) {
                    check_update_.setButtonText("failded");
                    check_update_.setEnabled(false);
                    juce::Timer::callAfterDelay(kButtonInfoDisplayTime, [this](){
                        check_update_.setButtonText("check update");
                        check_update_.setEnabled(true);
                    });
                }
                else {
                    check_update_.setButtonText("checking");
                    check_update_.setEnabled(false);
                }
            }
        }
    }
    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override {
        if (comboBoxThatHasChanged == &presetList) {
            if (presetList.getSelectedId() == 1) {
                presetManager.loadDefaultPatch();
            }
            else {
                presetManager.loadPreset(presetList.getItemText(presetList.getSelectedItemIndex()));
            }
        }
    }

    void configureButton(juce::Button& button, const juce::String& buttonText)  {
        button.setButtonText(buttonText);
        addAndMakeVisible(button);
        button.addListener(this);
    }

    void loadPresetList() {
        presetList.clear(juce::dontSendNotification);
        presetList.addSeparator();
        const auto allPresets = presetManager.getAllPresets();
        const auto currentPreset = presetManager.getCurrentPreset();
        presetList.addItemList(allPresets, 1);
        int idx = allPresets.indexOf(currentPreset);
        // if empty maybe a delete patch
        if (!currentPreset.isEmpty() && idx == -1) {
            // force default patch
            idx = 0;
        }
        presetList.setSelectedItemIndex(idx, juce::dontSendNotification);
    }

    void TempDisplayUpdateInfo(int ms, juce::StringRef text) {
        const juce::MessageManagerLock lock;
        juce::MessageManager::callAsync([this, text, ms]{
            check_update_.setButtonText(text);
            check_update_.setEnabled(false);
            juce::Timer::callAfterDelay(ms, [this](){
                check_update_.setButtonText("check update");
                check_update_.setEnabled(true);
            });
        });
    }

    friend class UpdateThread;
    class UpdateThread : public juce::Thread {
    public:
        UpdateThread(PresetPanel& panel)
            : juce::Thread("version check")
            , panel_(panel) {}

        void run() override {
            std::unique_ptr<juce::InputStream> stream;
            stream.reset(juce::URLInputSource{juce::URL{kReleaseJsonFile}}.createInputStream());
            if (!stream) {
                panel_.TempDisplayUpdateInfo(kButtonInfoDisplayTime, "network error");
                return;
            }

            auto version_string = stream->readEntireStreamAsString();
            auto json = juce::JSON::fromString(version_string);
            if (!json.isArray()) {
                panel_.TempDisplayUpdateInfo(kButtonInfoDisplayTime, "data error");
                return;
            }

            auto* array = json.getArray();
            if (!array) {
                panel_.TempDisplayUpdateInfo(kButtonInfoDisplayTime, "data error");
                return;
            }

            for (auto const& plugin : *array) {
                auto plugin_name = plugin.getProperty("name", "");
                if (!plugin_name.isString()) {
                    continue;
                }
                if (!plugin_name.toString().equalsIgnoreCase(JucePlugin_Name)) {
                    continue;
                }

                auto version = plugin.getProperty("version", JucePlugin_VersionString);
                if (!version.isString()) {
                    panel_.TempDisplayUpdateInfo(kButtonInfoDisplayTime, "data error");
                    return;
                }
                if (version.toString().equalsIgnoreCase(JucePlugin_VersionString)) {
                    panel_.TempDisplayUpdateInfo(kButtonInfoDisplayTime, "newest!");
                    return;
                }
                else {
                    juce::MessageManagerLock lock;
                    juce::MessageManager::callAsync([&p = panel_, v = version.toString()]{
                        juce::String s;
                        s << "new version: " << v;
                        p.check_update_.setButtonText(s);
                        p.have_new_version_ = true;
                        p.check_update_.setEnabled(true);
                    });
                    return;
                }
            }

            panel_.TempDisplayUpdateInfo(kButtonInfoDisplayTime, "plugin error");
        }
    private:
        PresetPanel& panel_;
    };

    PresetManager& presetManager;
    juce::TextButton saveButton, deleteButton, previousPresetButton, nextPresetButton;
    juce::ComboBox presetList;
    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::Label version_label_;
    juce::TextButton check_update_;
    std::unique_ptr<juce::Thread> update_thread_;
    bool have_new_version_{false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetPanel)
};
}
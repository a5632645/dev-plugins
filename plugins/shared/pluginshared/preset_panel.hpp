#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "component.hpp"
#include "preset_manager.hpp"
#include "update_gui.hpp"

namespace pluginshared {
class PresetPanel : public juce::Component, juce::Button::Listener {
public:
    inline static ui::CustomLookAndFeel look_and_feel;

    PresetPanel(PresetManager& pm)
        : presetManager(pm) {
        configureButton(saveButton, "Save");
        configureButton(deleteButton, "Delete");
        configureButton(previousPresetButton, "<");
        configureButton(nextPresetButton, ">");
        ui::SetLableBlack(preset_name_);
        preset_name_.addMouseListener(this, false);
        preset_name_.setText(presetManager.getCurrentPreset(), juce::dontSendNotification);
        addAndMakeVisible(preset_name_);
        preset_menu_.setLookAndFeel(&look_and_feel);
        loadPresetList();

        options_button_.setButtonText("options");
        addAndMakeVisible(options_button_);
        options_button_.addListener(this);
    }

    ~PresetPanel() override {
        saveButton.removeListener(this);
        deleteButton.removeListener(this);
        previousPresetButton.removeListener(this);
        nextPresetButton.removeListener(this);
        preset_name_.removeMouseListener(this);
        preset_menu_.setLookAndFeel(nullptr);
    }

    void mouseDown(const juce::MouseEvent& event) override {
        if (event.originalComponent != &preset_name_) return;
        preset_menu_.showMenuAsync(juce::PopupMenu::Options{}.withMousePosition(),
        [this](int id) {
            if (id == 1) {
                presetManager.loadDefaultPatch();
                preset_name_.setText(PresetManager::kDefaultPresetName, juce::dontSendNotification);
            }
            else if (id > 1) {
                const auto& all_items = presetManager.getAllPresets();
                int idx = id - 2;
                if (idx >= all_items.size()) return;

                const auto& name = all_items[idx];
                presetManager.loadPreset(name);
                preset_name_.setText(name, juce::dontSendNotification);
            }
        });
    }

    void resized() override {
        auto container = getLocalBounds();

        options_button_.setBounds(container.removeFromLeft(container.proportionOfWidth(0.15f)).reduced(2));

        auto const b = container;
        deleteButton.setBounds(container.removeFromRight(b.proportionOfWidth(0.15f)).reduced(2));
        saveButton.setBounds(container.removeFromRight(b.proportionOfWidth(0.15f)).reduced(2));
        previousPresetButton.setBounds(container.removeFromLeft(container.getHeight()).reduced(2));
        nextPresetButton.setBounds(container.removeFromRight(container.getHeight()).reduced(2));
        preset_name_.setBounds(container.reduced(2));
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(ui::green_bg);
    }

    std::function<void(juce::PopupMenu&)> on_menu_showup;
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
                const auto preset_name = resultFile.getFileNameWithoutExtension();
                if (preset_name == PresetManager::kDefaultPresetName) {
                    juce::NativeMessageBox::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "Invalid name", "default preset name is invalid");
                    return;
                }
                presetManager.savePreset(preset_name);
                loadPresetList();
            });
        }
        else if (button == &previousPresetButton) {
            auto[index, name] = presetManager.loadPreviousPreset();
            preset_name_.setText(name, juce::dontSendNotification);
        }
        else if (button == &nextPresetButton) {
            auto[index, name] = presetManager.loadNextPreset();
            preset_name_.setText(name, juce::dontSendNotification);
        }
        else if (button == &deleteButton) {
            presetManager.deletePreset(presetManager.getCurrentPreset());
            loadPresetList();
            preset_name_.setText(presetManager.getCurrentPreset(), juce::dontSendNotification);
        }
        else if (button == &options_button_) {
            juce::PopupMenu menu;

            juce::String plugin_name;
            plugin_name << JucePlugin_Name << ' ' << JucePlugin_VersionString;
            menu.addItem(plugin_name, false, false, []{});

            if (presetManager.GetUpdateData().HaveNewVersion()) {
                menu.addItem("new version", []{
                    juce::URL{UpdateData::kReleasePageURL}.launchInDefaultBrowser();
                });
            }
            else {
                menu.addItem("check update", [this]{
                    CheckUpdate();
                });
            }

            menu.addItem("init patch", [this]{
                preset_name_.setText(PresetManager::kDefaultPresetName, juce::dontSendNotification);
                presetManager.loadDefaultPatch();
            });

            // scale
            juce::PopupMenu scale_menu;
            scale_menu.addItem("100%", [this] { TrySetParentScale(1.0f); });
            scale_menu.addItem("125%", [this] { TrySetParentScale(1.25f); });
            scale_menu.addItem("150%", [this] { TrySetParentScale(1.5f); });
            scale_menu.addItem("175%", [this] { TrySetParentScale(1.75f); });
            scale_menu.addItem("200%", [this] { TrySetParentScale(2.0f); });
            scale_menu.addItem("300%", [this] { TrySetParentScale(3.0f); });
            menu.addSubMenu("scale", std::move(scale_menu));

            if (on_menu_showup) {
                on_menu_showup(menu);
            }

            juce::PopupMenu::Options op;
            menu.showMenuAsync(op.withMousePosition());
        }
    }

    void TrySetParentScale(float scale) {
        auto* editor = findParentComponentOfClass<juce::AudioProcessorEditor>();
        if (editor != nullptr) {
            editor->setScaleFactor(scale);
        }
    }

    void configureButton(juce::Button& button, const juce::String& buttonText)  {
        button.setButtonText(buttonText);
        addAndMakeVisible(button);
        button.addListener(this);
    }

    void loadPresetList() {
        const auto currentPreset = presetManager.getCurrentPreset();
        preset_name_.setText(currentPreset, juce::dontSendNotification);
        const auto& allPresets = presetManager.getAllPresets();
        preset_menu_.clear();
        preset_menu_.addItem(1, "init patch");
        preset_menu_.addSeparator();
        for (int id = 2; const auto& name : allPresets) {
            preset_menu_.addItem(id++, name);
        }
    }

    void CheckUpdate() {
        presetManager.GetUpdateData().BeginCheck();
        auto* diaglog = new UpdateMessageDialog(presetManager.GetUpdateData());
        diaglog->enterModalState(true, nullptr, true);
    }

    PresetManager& presetManager;
    ui::FlatButton saveButton, deleteButton, previousPresetButton, nextPresetButton;
    juce::Label preset_name_{"", PresetManager::kDefaultPresetName};
    juce::PopupMenu preset_menu_;
    std::unique_ptr<juce::FileChooser> fileChooser;
    ui::FlatButton options_button_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetPanel)
};
}

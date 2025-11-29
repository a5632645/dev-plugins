#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "juce_events/juce_events.h"
#include "preset_manager.hpp"
#include "pluginshared/component.hpp"

namespace pluginshared {
class PresetPanel : public juce::Component, juce::Button::Listener {
public:
    static constexpr int kNetworkTimeout = 500; // ms
    static constexpr auto kReleasePageURL = "https://github.com/a5632645/dev-plugins/releases";
    static constexpr auto kReleaseJsonFile = "https://raw.githubusercontent.com/a5632645/dev-plugins/refs/heads/main/release.json";
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

            if (presetManager.have_new_version_) {
                menu.addItem("new version", []{
                    juce::URL{kReleasePageURL}.launchInDefaultBrowser();
                });
            }
            else {
                menu.addItem("check update", [this]{
                    CheckUpdate();
                });
            }

            menu.addSeparator();
            menu.addItem("init patch", [this]{
                preset_name_.setText(PresetManager::kDefaultPresetName, juce::dontSendNotification);
                presetManager.loadDefaultPatch();
            });

            if (on_menu_showup) {
                on_menu_showup(menu);
            }

            juce::PopupMenu::Options op;
            menu.showMenuAsync(op.withMousePosition());
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
        if (presetManager.update_thread_) {
            presetManager.update_thread_->stopThread(-1);
            presetManager.update_thread_ = nullptr;
        }
        presetManager.update_thread_ = std::make_unique<UpdateThread>(*this, presetManager);
        if (!presetManager.update_thread_->startThread()) {
            juce::NativeMessageBox::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "check update error", "launch update thread failed");
            return;
        }

        auto* diaglog = new UpdateMessageDialog;

        update_message_.label.setText("checking update", juce::dontSendNotification);
        update_message_.button.setButtonText("cancel");
        update_message_.button.onClick = [diaglog] {
            diaglog->userTriedToCloseWindow();
        };
        diaglog->on_close = [this] {
            if (presetManager.update_thread_) {
                presetManager.update_thread_->stopThread(-1);
                presetManager.update_thread_ = nullptr;
            }
            update_message_.button.onClick = []{};
        };

        diaglog->setContentNonOwned(&update_message_, true);
        diaglog->setTopLeftPosition(juce::Desktop::getInstance().getMousePosition());
        diaglog->enterModalState(true, nullptr, true);
    }

    class UpdateMessageDialog : public juce::DialogWindow {
    public:
        UpdateMessageDialog()
            : juce::DialogWindow("checking update", juce::Colours::black, false) {}

        void closeButtonPressed() override {
            if (on_close) {
                on_close();
            }
            delete this;
        }

        std::function<void()> on_close;
    };

    class UpdateMessageComponent : public juce::Component {
    public:
        UpdateMessageComponent() {
            addAndMakeVisible(label);
            label.setJustificationType(juce::Justification::topLeft);
            addAndMakeVisible(button);
            setSize(300, 150);
        }

        void resized() override {
            auto b = getLocalBounds();
            label.setBounds(b.removeFromTop(b.proportionOfHeight(0.8f)));
            button.setBounds(b);
        }

        juce::Label label;
        juce::TextButton button;
    };

    friend class UpdateThread;
    class UpdateThread : public juce::Thread {
    public:
        UpdateThread(PresetPanel& panel, PresetManager& manager)
            : juce::Thread("version check")
            , panel_(&panel)
            , manager_(manager) {}

        void run() override {
            juce::URL::InputStreamOptions op{juce::URL::ParameterHandling::inAddress};
            // why not give me a std::future like class, then i can run a loop check
            auto stream = juce::URL{kReleaseJsonFile}
                .createInputStream(op.withConnectionTimeoutMs(kNetworkTimeout));

            if (threadShouldExit()) {
                return;
            }

            if (!stream) {
                SetUpdateMessage("network error", "ok");
                return;
            }

            auto version_string = stream->readEntireStreamAsString();
            auto json = juce::JSON::fromString(version_string);
            if (!json.isArray()) {
                SetUpdateMessage("payload error", "ok");
                return;
            }

            auto* array = json.getArray();
            if (!array) {
                SetUpdateMessage("payload error", "ok");
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
                    SetUpdateMessage("payload error", "ok");
                    return;
                }
                if (version.toString().equalsIgnoreCase(JucePlugin_VersionString)) {
                    SetUpdateMessage("you are using newest plugin", "ok");
                    return;
                }
                else {
                    manager_.have_new_version_ = true;
                    if (!panel_) {
                        return;
                    }

                    auto changelog = plugin.getProperty("changelog", "").toString();
                    juce::String s;
                    s << "new version avaliable: " << version.toString() << "\n";
                    s << "chanelog: " << changelog;

                    juce::MessageManagerLock lock;
                    juce::MessageManager::callAsync([p = panel_, text = std::move(s)]{
                        p->update_message_.label.setText(text, juce::dontSendNotification);
                        p->update_message_.button.setButtonText("ok");
                    });
                    return;
                }
            }

            SetUpdateMessage("plugin error", "ok");
        }
    private:
        void SetUpdateMessage(juce::StringRef label, juce::StringRef button) {
            if (!panel_) {
                return;
            }

            juce::MessageManagerLock lock;
            juce::MessageManager::callAsync([p = panel_, label, button]{
                p->update_message_.label.setText(label, juce::dontSendNotification);
                p->update_message_.button.setButtonText(button);
            });
        }

        juce::Component::SafePointer<PresetPanel> panel_;
        PresetManager& manager_;
    };

    PresetManager& presetManager;
    ui::FlatButton saveButton, deleteButton, previousPresetButton, nextPresetButton;
    // ui::FlatCombobox presetList;
    juce::Label preset_name_{"", PresetManager::kDefaultPresetName};
    juce::PopupMenu preset_menu_;
    std::unique_ptr<juce::FileChooser> fileChooser;
    ui::FlatButton options_button_;
    UpdateMessageComponent update_message_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetPanel)
};
}

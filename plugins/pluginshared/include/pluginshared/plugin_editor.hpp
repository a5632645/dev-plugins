#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "component.hpp"

namespace pluginshared {

/**
 * @brief Base class for plugin editors with size-preserving scaling.
 *
 * Handles:
 * - PropertiesFile-based scale persistence
 * - Uniform aspect-ratio scaling via transform
 * - Resize constraints
 *
 * @tparam Processor  The AudioProcessor subclass
 * @tparam Ui         The UI Component subclass
 */
template <typename Processor, typename Ui>
class PluginEditorBase : public juce::AudioProcessorEditor {
public:
    PluginEditorBase(Processor& p)
        : AudioProcessorEditor(&p)
        , ui_(p) {
        ui::ColorPresetManager::LoadColorsFromConfig();
        auto ui_bound = ui_.getLocalBounds();
        jassert(!ui_bound.isEmpty() && "you must set an editor size");

        ui_width_ = ui_bound.getWidth();
        ui_height_ = ui_bound.getHeight();

        if (auto* props = plugin_config_->config.get()) {
            scale_ = static_cast<float>(props->getDoubleValue("scale", 1.0));
            setSize(static_cast<int>(static_cast<float>(ui_width_) * scale_),
                    static_cast<int>(static_cast<float>(ui_height_) * scale_));
        } else {
            setSize(ui_width_, ui_height_);
        }

        setResizable(true, true);
        getConstrainer()->setFixedAspectRatio(
            static_cast<float>(ui_width_) / static_cast<float>(ui_height_));
        setResizeLimits(ui_width_, ui_height_, 9999, 9999);
        addAndMakeVisible(ui_);
    }

    ~PluginEditorBase() override = default;

    void paint(juce::Graphics& g) override {
        g.fillAll(ui::green_bg);
    }

    void resized() override {
        scale_ = static_cast<float>(getWidth()) / static_cast<float>(ui_width_);
        ui_.setTransform(juce::AffineTransform::scale(scale_, scale_));

        if (auto* props = plugin_config_->config.get()) {
            if (getWidth() > 0 && getHeight() > 0) {
                props->setValue("scale", static_cast<double>(scale_));
            }
        }
    }

    // Change the logical UI size and re-scale the window.
    void SetNewSize(int width, int height) {
        ui_width_ = width;
        ui_height_ = height;
        ui_.setSize(width, height);
        getConstrainer()->setFixedAspectRatio(
            static_cast<float>(ui_width_) / static_cast<float>(ui_height_));
        setSize(static_cast<int>(static_cast<float>(width) * scale_),
                static_cast<int>(static_cast<float>(height) * scale_));
    }

    Processor& GetProcessor() {
        return static_cast<Processor&>(*getAudioProcessor());
    }

protected:
    Ui ui_;

private:
    struct PluginConfig {
        PluginConfig() {
            juce::PropertiesFile::Options options{};
            options.applicationName = JucePlugin_Name;
            options.filenameSuffix = ".settings";
#if defined(JUCE_LINUX) || defined(JUCE_BSD)
            options.folderName = "~/.config/" JucePlugin_Name;
#elif defined(JUCE_MAC) || defined(JUCE_IOS)
            options.folderName = JucePlugin_Name;
#endif
            options.osxLibrarySubFolder = "Application Support";
            options.storageFormat = juce::PropertiesFile::storeAsXML;
            config = std::make_unique<juce::PropertiesFile>(options);
        }

        std::unique_ptr<juce::PropertiesFile> config;
    };

    juce::SharedResourcePointer<PluginConfig> plugin_config_;
    float scale_{1.0f};
    int ui_width_{};
    int ui_height_{};
};

} // namespace pluginshared

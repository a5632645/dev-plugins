#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace pluginshared {
/**
 * @note must create after building AudioProcessorValueTreeState
 */
class PresetManager : juce::ValueTree::Listener
{
public:
    inline static const juce::File defaultDirectory{ juce::File::getSpecialLocation(
    juce::File::SpecialLocationType::userDocumentsDirectory)
        .getChildFile(JucePlugin_Manufacturer)
        .getChildFile(JucePlugin_Name)
    };
    inline static const juce::String extension{"preset"};
    inline static const juce::String presetNameProperty{"presetName"};
    inline static const juce::String kVersionProperty{"version"};

    PresetManager(juce::AudioProcessorValueTreeState& apvts, juce::AudioProcessor& p) 
        : valueTreeState(apvts)
        , processor_(p)
    {
        // Create a default Preset Directory, if it doesn't exist
        if (!defaultDirectory.exists())
        {
            const auto result = defaultDirectory.createDirectory();
            if (result.failed())
            {
                DBG("Could not create preset directory: " + result.getErrorMessage());
                jassertfalse;
            }
        }

        apvts.state.setProperty(presetNameProperty, "", nullptr);
        apvts.state.setProperty("version", JucePlugin_VersionString, nullptr);

        valueTreeState.state.addListener(this);
        currentPreset.referTo(valueTreeState.state.getPropertyAsValue(presetNameProperty, nullptr));

        default_state_ = valueTreeState.copyState();
    }

    ~PresetManager() override {
        if (update_thread_) {
            update_thread_->stopThread(-1);
            update_thread_ = nullptr;
        }
    }

    void savePreset(const juce::String& presetName)
    {
        if (presetName.isEmpty())
            return;

        currentPreset.setValue(presetName);

        juce::MemoryBlock block;
        processor_.getStateInformation(block);
        const auto presetFile = defaultDirectory.getChildFile(presetName + "." + extension);
        if (presetFile.existsAsFile()) {
            presetFile.deleteFile();
        }
        
        juce::FileOutputStream stream{presetFile};
        if (!stream.write(block.getData(), block.getSize()))
        {
            DBG("Could not create preset file: " + presetFile.getFullPathName());
            jassertfalse;
        }
    }

    void deletePreset(const juce::String& presetName)
    {
        if (presetName.isEmpty())
            return;

        const auto presetFile = defaultDirectory.getChildFile(presetName + "." + extension);
        if (!presetFile.existsAsFile())
        {
            DBG("Preset file " + presetFile.getFullPathName() + " does not exist");
            jassertfalse;
            return;
        }
        if (!presetFile.deleteFile())
        {
            DBG("Preset file " + presetFile.getFullPathName() + " could not be deleted");
            jassertfalse;
            return;
        }
        currentPreset.setValue("");
    }

    void loadPreset(const juce::String& presetName)
    {
        if (presetName.isEmpty()) {
            return;
        }

        const auto presetFile = defaultDirectory.getChildFile(presetName + "." + extension);
        if (!presetFile.existsAsFile())
        {
            DBG("Preset file " + presetFile.getFullPathName() + " does not exist");
            jassertfalse;
            return;
        }
        // presetFile (XML) -> (ValueTree)
        juce::FileInputStream c{presetFile};
        if (c.failedToOpen()) {
            return;
        }

        juce::MemoryBlock block;
        c.readIntoMemoryBlock(block);
        processor_.setStateInformation(block.getData(), static_cast<int>(block.getSize()));

        currentPreset.setValue(presetName);
    }

    int loadNextPreset()
    {
        const auto allPresets = getAllPresets();
        if (allPresets.isEmpty()) {
            loadDefaultPatch();
            return -1;
        }
        const auto currentIndex = allPresets.indexOf(currentPreset.toString());
        const auto nextIndex = currentIndex + 1 > (allPresets.size() - 1) ? 0 : currentIndex + 1;
        loadPreset(allPresets.getReference(nextIndex));
        return nextIndex;
    }

    int loadPreviousPreset()
    {
        const auto allPresets = getAllPresets();
        if (allPresets.isEmpty()) {
            loadDefaultPatch();
            return -1;
        }
        const auto currentIndex = allPresets.indexOf(currentPreset.toString());
        const auto previousIndex = currentIndex - 1 < 0 ? allPresets.size() - 1 : currentIndex - 1;
        loadPreset(allPresets.getReference(previousIndex));
        return previousIndex;
    }

    juce::StringArray getAllPresets() const
    {
        juce::StringArray presets;
        const auto fileArray = defaultDirectory.findChildFiles(
            juce::File::TypesOfFileToFind::findFiles, false, "*." + extension);
        for (const auto& file : fileArray)
        {
            presets.add(file.getFileNameWithoutExtension());
        }
        return presets;
    }

    juce::String getCurrentPreset() const
    {
        return currentPreset.toString();
    }

    void loadDefaultPatch() {
        processor_.suspendProcessing(true);
        valueTreeState.replaceState(default_state_.createCopy());
        if (external_load_default_operations) {
            external_load_default_operations();
        }
        processor_.suspendProcessing(false);
    }

    juce::ValueTree GetDefaultValueTree() {
        return default_state_;
    }
    
    /**
     * @brief make audio processor goes into default state, value tree is automatic done
     * @note when call this, the processor will automatic suspend
     */
    std::function<void()> external_load_default_operations;
private:
    void valueTreeRedirected(juce::ValueTree& treeWhichHasBeenChanged) override
    {
        currentPreset.referTo(treeWhichHasBeenChanged.getPropertyAsValue(presetNameProperty, nullptr));
    }

    juce::AudioProcessorValueTreeState& valueTreeState;
    juce::AudioProcessor& processor_;
    juce::Value currentPreset;
    juce::ValueTree default_state_;
    std::unique_ptr<juce::Thread> update_thread_;
    std::atomic<bool> have_new_version_{false};

    friend class PresetPanel;
};
}

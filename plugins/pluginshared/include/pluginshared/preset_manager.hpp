#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "update_data.hpp"

namespace pluginshared {
/**
 * @note must create after building AudioProcessorValueTreeState
 */
class PresetManager : juce::ValueTree::Listener {
public:
    inline static const juce::File defaultDirectory{
        juce::File::getSpecialLocation(juce::File::SpecialLocationType::userDocumentsDirectory)
            .getChildFile(JucePlugin_Manufacturer)
            .getChildFile(JucePlugin_Name)};
    inline static const juce::String extension{"xml"};
    inline static const juce::String presetNameProperty{"presetName"};
    inline static const juce::String kVersionProperty{"version"};
    inline static const juce::String kDefaultPresetName = "default";
    inline static const juce::String kFactoryPresetAutoNamePrefix = "Factory ";

    enum class PresetScope {
        kDefault,
        kFactory,
        kUser,
    };

    PresetManager(juce::AudioProcessorValueTreeState& apvts, juce::AudioProcessor& p)
        : valueTreeState(apvts)
        , processor_(p) {
        // Create a default Preset Directory, if it doesn't exist
        if (!defaultDirectory.exists()) {
            const auto result = defaultDirectory.createDirectory();
            if (result.failed()) {
                DBG("Could not create preset directory: " + result.getErrorMessage());
            }
        }

        // Migrate old .preset files to .xml
        const auto oldFiles =
            defaultDirectory.findChildFiles(juce::File::TypesOfFileToFind::findFiles, false, "*.preset");
        for (const auto& f : oldFiles) {
            const auto newFile = f.getParentDirectory().getChildFile(f.getFileNameWithoutExtension() + "." + extension);
            f.moveFileTo(newFile);
        }

        apvts.state.setProperty(presetNameProperty, kDefaultPresetName, nullptr);
        apvts.state.setProperty(kVersionProperty, JucePlugin_VersionString, nullptr);

        valueTreeState.state.addListener(this);
        currentPreset.referTo(valueTreeState.state.getPropertyAsValue(presetNameProperty, nullptr));
        p.getCurrentProgramStateInformation(default_state_block_);
    }

    void AddFactoryPreset(const char* xml, int xml_size, const juce::String& name) {
        factory_presets_.push_back({name, xml, xml_size});
    }

    void savePreset(const juce::String& presetName) {
        if (presetName.isEmpty() || presetName == kDefaultPresetName) return;

        currentPreset.setValue(presetName);

        juce::MemoryBlock block;
        processor_.getStateInformation(block);
        const auto newFile = defaultDirectory.getChildFile(presetName + "." + extension);
        if (newFile.existsAsFile()) {
            newFile.deleteFile();
        }

        juce::FileOutputStream stream{newFile};
        if (!stream.write(block.getData(), block.getSize())) {
            DBG("Could not create preset file: " + newFile.getFullPathName());
        }

        current_scope_ = PresetScope::kUser;
        current_user_preset_index_ = getUserPresetNames().indexOf(presetName);
        current_factory_preset_index_ = -1;
    }

    void deletePreset(const juce::String& presetName) {
        if (presetName.isEmpty() || presetName == kDefaultPresetName) return;

        const auto presetFile = defaultDirectory.getChildFile(presetName + "." + extension);
        if (!presetFile.existsAsFile()) {
            DBG("Preset file does not exist for: " + presetName);
            return;
        }
        if (!presetFile.deleteFile()) {
            DBG("Preset file " + presetFile.getFullPathName() + " could not be deleted");
            return;
        }
        currentPreset.setValue("*deleted*");
        current_scope_ = PresetScope::kDefault;
        current_factory_preset_index_ = -1;
        current_user_preset_index_ = -1;
    }

    void loadPreset(const juce::String& presetName) {
        if (presetName.isEmpty()) {
            return;
        }

        const auto presetFile = defaultDirectory.getChildFile(presetName + "." + extension);
        if (!presetFile.existsAsFile()) {
            DBG("Preset file does not exist for: " + presetName);
            return;
        }

        juce::FileInputStream c{presetFile};
        if (c.failedToOpen()) {
            return;
        }

        juce::MemoryBlock block;
        c.readIntoMemoryBlock(block);
        processor_.setStateInformation(block.getData(), static_cast<int>(block.getSize()));

        currentPreset.setValue(presetName);
        current_scope_ = PresetScope::kUser;
        current_user_preset_index_ = getUserPresetNames().indexOf(presetName);
        current_factory_preset_index_ = -1;
    }

    bool loadUserPresetByIndex(int index) {
        const auto userPresets = getUserPresetNames();
        if (index < 0 || index >= userPresets.size()) {
            return false;
        }
        loadPreset(userPresets[index]);
        current_scope_ = PresetScope::kUser;
        current_user_preset_index_ = index;
        current_factory_preset_index_ = -1;
        return true;
    }

    bool loadFactoryPresetByIndex(int index) {
        if (index < 0 || index >= static_cast<int>(factory_presets_.size())) {
            return false;
        }

        const auto& preset = factory_presets_[static_cast<size_t>(index)];
        processor_.setStateInformation(preset.xml, preset.xml_size);

        const auto& name = preset.name;
        currentPreset.setValue(name);
        current_scope_ = PresetScope::kFactory;
        current_factory_preset_index_ = index;
        current_user_preset_index_ = -1;
        return true;
    }

    std::pair<int, juce::String> loadNextPreset() {
        if (current_scope_ == PresetScope::kFactory) {
            return loadNextFactoryPreset();
        }
        if (current_scope_ == PresetScope::kUser) {
            return loadNextUserPreset();
        }

        if (!factory_presets_.empty() && loadFactoryPresetByIndex(0)) {
            return {0, getCurrentPreset()};
        }

        const auto userPresets = getUserPresetNames();
        if (!userPresets.isEmpty() && loadUserPresetByIndex(0)) {
            return {0, getCurrentPreset()};
        }

        return {-1, getCurrentPreset()};
    }

    std::pair<int, juce::String> loadPreviousPreset() {
        if (current_scope_ == PresetScope::kFactory) {
            return loadPreviousFactoryPreset();
        }
        if (current_scope_ == PresetScope::kUser) {
            return loadPreviousUserPreset();
        }

        if (!factory_presets_.empty() && loadFactoryPresetByIndex(0)) {
            return {0, getCurrentPreset()};
        }

        const auto userPresets = getUserPresetNames();
        if (!userPresets.isEmpty() && loadUserPresetByIndex(0)) {
            return {0, getCurrentPreset()};
        }

        return {-1, getCurrentPreset()};
    }

    juce::StringArray getFactoryPresetNames() const {
        juce::StringArray presets;
        for (const auto& preset : factory_presets_) {
            presets.add(preset.name);
        }
        return presets;
    }

    juce::StringArray getUserPresetNames() const {
        return getAllPresets();
    }

    juce::StringArray getAllPresets() const {
        juce::StringArray presets;
        const auto fileArray =
            defaultDirectory.findChildFiles(juce::File::TypesOfFileToFind::findFiles, false, "*." + extension);
        for (const auto& file : fileArray) {
            presets.add(file.getFileNameWithoutExtension());
        }
        return presets;
    }

    juce::String getCurrentPreset() const {
        return currentPreset.toString();
    }

    void loadDefaultPatch() {
        processor_.setStateInformation(default_state_block_.getData(),
                                       static_cast<int>(default_state_block_.getSize()));
        currentPreset.setValue(kDefaultPresetName);
        current_scope_ = PresetScope::kDefault;
        current_factory_preset_index_ = -1;
        current_user_preset_index_ = -1;
        if (external_load_default_operations) {
            external_load_default_operations();
        }
    }

    UpdateData& GetUpdateData() {
        return update_data_;
    }

    /**
     * @brief make audio processor goes into default state, value tree is automatic done
     * @note when call this, the processor will automatic suspend
     */
    std::function<void()> external_load_default_operations;
private:
    std::pair<int, juce::String> loadNextFactoryPreset() {
        if (factory_presets_.empty()) {
            return {-1, getCurrentPreset()};
        }

        int currentIndex = current_factory_preset_index_;
        if (currentIndex < 0 || currentIndex >= static_cast<int>(factory_presets_.size())) {
            currentIndex = -1;
        }
        const int nextIndex = (currentIndex + 1) % static_cast<int>(factory_presets_.size());
        if (loadFactoryPresetByIndex(nextIndex)) {
            return {nextIndex, getCurrentPreset()};
        }
        return {-1, getCurrentPreset()};
    }

    std::pair<int, juce::String> loadPreviousFactoryPreset() {
        if (factory_presets_.empty()) {
            return {-1, getCurrentPreset()};
        }

        int currentIndex = current_factory_preset_index_;
        if (currentIndex < 0 || currentIndex >= static_cast<int>(factory_presets_.size())) {
            currentIndex = 0;
        }
        const int size = static_cast<int>(factory_presets_.size());
        const int previousIndex = (currentIndex - 1 + size) % size;
        if (loadFactoryPresetByIndex(previousIndex)) {
            return {previousIndex, getCurrentPreset()};
        }
        return {-1, getCurrentPreset()};
    }

    std::pair<int, juce::String> loadNextUserPreset() {
        const auto userPresets = getUserPresetNames();
        if (userPresets.isEmpty()) {
            return {-1, getCurrentPreset()};
        }

        int currentIndex = current_user_preset_index_;
        if (currentIndex < 0 || currentIndex >= userPresets.size()) {
            currentIndex = userPresets.indexOf(currentPreset.toString());
            if (currentIndex < 0) currentIndex = -1;
        }
        const int nextIndex = (currentIndex + 1) % userPresets.size();
        if (loadUserPresetByIndex(nextIndex)) {
            return {nextIndex, getCurrentPreset()};
        }
        return {-1, getCurrentPreset()};
    }

    std::pair<int, juce::String> loadPreviousUserPreset() {
        const auto userPresets = getUserPresetNames();
        if (userPresets.isEmpty()) {
            return {-1, getCurrentPreset()};
        }

        int currentIndex = current_user_preset_index_;
        if (currentIndex < 0 || currentIndex >= userPresets.size()) {
            currentIndex = userPresets.indexOf(currentPreset.toString());
            if (currentIndex < 0) currentIndex = 0;
        }
        const int size = userPresets.size();
        const int previousIndex = (currentIndex - 1 + size) % size;
        if (loadUserPresetByIndex(previousIndex)) {
            return {previousIndex, getCurrentPreset()};
        }
        return {-1, getCurrentPreset()};
    }

    void valueTreeRedirected(juce::ValueTree& treeWhichHasBeenChanged) override {
        currentPreset.referTo(treeWhichHasBeenChanged.getPropertyAsValue(presetNameProperty, nullptr));
        current_scope_ = PresetScope::kDefault;
        current_factory_preset_index_ = -1;
        current_user_preset_index_ = -1;
    }

    juce::AudioProcessorValueTreeState& valueTreeState;
    juce::MemoryBlock default_state_block_;
    juce::AudioProcessor& processor_;
    juce::Value currentPreset;

    UpdateData update_data_;

    struct FactoryPreset {
        juce::String name;
        const char* xml;
        int xml_size;
    };
    std::vector<FactoryPreset> factory_presets_;
    int current_factory_preset_index_{-1};
    int current_user_preset_index_{-1};
    PresetScope current_scope_{PresetScope::kDefault};

    friend class PresetPanel;
};
} // namespace pluginshared

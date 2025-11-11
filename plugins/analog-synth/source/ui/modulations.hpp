#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <pluginshared/component.hpp>

class AnalogSynthAudioProcessor;

namespace analogsynth {
class Synth;
struct ModulationInfo;

class ModulationMatrixLayout : public juce::Component
    , public juce::TableListBoxModel
    , private juce::Timer {
public:
    ModulationMatrixLayout(AnalogSynthAudioProcessor& p);
    ~ModulationMatrixLayout() override;

    void resized() override;
private:
    void OnAddButtonClick();
    void timerCallback() override;
    void cellClicked (int rowNumber, int columnId, const juce::MouseEvent& mouseEvent) override;

    AnalogSynthAudioProcessor& processor_;
    Synth& synth_;
    std::unique_ptr<juce::TableListBox> table_;
    std::vector<ModulationInfo*> local_modulations_;

    ui::FlatCombobox modulators_;
    ui::FlatCombobox parameters_;
    ui::FlatButton add_;

    // 通过 TableListBoxModel 继承
    int getNumRows() override;
    void paintRowBackground(juce::Graphics&, int rowNumber, int width, int height, bool rowIsSelected) override;
    void paintCell(juce::Graphics&, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
    juce::Component* refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected, juce::Component* existingComponentToUpdate) override;
};
}
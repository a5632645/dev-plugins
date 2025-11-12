#include "modulations.hpp"

#include "../dsp/synth.hpp"
#include "../PluginProcessor.h"

namespace analogsynth {
// -------------------- toggle box --------------------
class ModuWrapToggleBox : public juce::ToggleButton, public juce::Button::Listener {
public:
    ModuWrapToggleBox(ModulationInfo* cfg) {
        this->addListener(this);
        SetModulationConfig(cfg);
    }

    ~ModuWrapToggleBox() override {
        removeListener(this);
    }

    void SetModulationConfig(ModulationInfo* cfg) {
        if (config_ == cfg)
            return;

        config_ = cfg;
        setToggleState(cfg->enable, juce::dontSendNotification);
    }
private:
    ModulationInfo* config_;

    // 通过 Listener 继承
    void buttonClicked(Button*) override {
        if (config_ == nullptr) return;
        config_->enable = getToggleState();
    }
};

// -------------------- amount slider --------------------
class ModuWrapAmountSlider : public juce::Slider {
public:
    ModuWrapAmountSlider(ModulationInfo* cfg) {
        setRange(-1.0f, 1.0f);
        setDoubleClickReturnValue(true, 0.0);
        this->onValueChange = [this]() {
            if (config_ == nullptr) return;
            config_->amount = static_cast<float>(getValue());
        };
        SetModulationConfig(cfg);
    }

    ~ModuWrapAmountSlider() override {
    }

    void SetModulationConfig(ModulationInfo* cfg) {
        if (config_ == cfg)
            return;

        setValue(cfg->amount, juce::dontSendNotification);
        config_ = cfg;
    }

private:
    ModulationInfo* config_;
};

// -------------------- toggle box2 --------------------
class ModuWrapBipolarToggleBox : public juce::ToggleButton {
public:
    ModuWrapBipolarToggleBox(ModulationInfo* cfg) {
        this->onStateChange = [this]() {
            if (config_ == nullptr) return;
            config_->bipolar = getToggleState();
        };
        SetModulationConfig(cfg);
    }

    ~ModuWrapBipolarToggleBox() override {
    }

    void SetModulationConfig(ModulationInfo* cfg) {
        if (config_ == cfg)
            return;

        setToggleState(cfg->bipolar, juce::dontSendNotification);
        config_ = cfg;
    }

private:
    ModulationInfo* config_;
};

// ----------------------------------------
// modulation gui
// ----------------------------------------
ModulationMatrixLayout::ModulationMatrixLayout(AnalogSynthAudioProcessor& p)
    : processor_(p)
    , synth_(p.synth_) {
    table_ = std::make_unique<juce::TableListBox>("table", this);
    table_->getHeader().addColumn("modulator", 1, 35);
    table_->getHeader().addColumn("param", 2, 120);
    table_->getHeader().addColumn("enable", 3, 30);
    table_->getHeader().addColumn("bipolar", 4, 30);
    table_->getHeader().addColumn("amount", 5, 260);
    addAndMakeVisible(*table_);

    add_.setButtonText("add");
    add_.onClick = [this] {
        OnAddButtonClick();
    };
    addAndMakeVisible(add_);

    for (int item_id = 1; auto&[a,b] : synth_.modulation_matrix.modulator_hash_table_) {
        modulators_.addItem(a, item_id++);
    }
    addAndMakeVisible(modulators_);

    for (int item_id = 1; auto&[a,b] : synth_.modulation_matrix.parameter_hash_table_) {
        parameters_.addItem(a, item_id++);
    }
    addAndMakeVisible(parameters_);

    startTimerHz(30);
}

ModulationMatrixLayout::~ModulationMatrixLayout() {
    table_->setModel(nullptr);
    table_ = nullptr;
}

void ModulationMatrixLayout::resized() {
    auto b = getLocalBounds();
    auto button_bound = b.removeFromTop(30);
    add_.setBounds(button_bound.removeFromRight(button_bound.proportionOfWidth(0.15f)));
    modulators_.setBounds(button_bound.removeFromLeft(button_bound.proportionOfWidth(0.15f)));
    parameters_.setBounds(button_bound);
    table_->setBounds(b);
}

int ModulationMatrixLayout::getNumRows() {
    return static_cast<int>(local_modulations_.size());
}

void ModulationMatrixLayout::paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) {
    std::ignore = width = height = rowIsSelected;
    auto alternateColour = getLookAndFeel().findColour(juce::ListBox::backgroundColourId)
        .interpolatedWith(getLookAndFeel().findColour(juce::ListBox::textColourId), 0.03f);
    if (rowIsSelected)
        g.fillAll(ui::light_green_bg);
    else if (rowNumber % 2)
        g.fillAll(alternateColour);
}

void ModulationMatrixLayout::paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) {
    std::ignore = rowIsSelected;
    if (columnId != 1 && columnId != 2) {
        return;
    }

    if (rowNumber >= getNumRows()) {
        return;
    }

    auto config = local_modulations_[size_t(rowNumber)];
    g.setColour(juce::Colours::white);
    if (columnId == 1) {
        g.drawText(config->source->name_, 2, 0, width - 4, height, juce::Justification::centredLeft, true);
    }
    else if (columnId == 2) {
        g.drawText(config->target->name_, 2, 0, width - 4, height, juce::Justification::centredLeft, true);
    }
}

juce::Component* ModulationMatrixLayout::refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected, juce::Component* existingComponentToUpdate) {
    juce::ignoreUnused(isRowSelected);
    if (rowNumber >= getNumRows()) {
        return nullptr;
    }

    auto config = local_modulations_[size_t(rowNumber)];
    if (columnId == 3) { // enable
        if (existingComponentToUpdate == nullptr) {
            return new ModuWrapToggleBox(config);
        }
        else {
            static_cast<ModuWrapToggleBox*>(existingComponentToUpdate)->SetModulationConfig(config);
            return existingComponentToUpdate;
        }
    }
    else if (columnId == 4) { // bipolar
        if (existingComponentToUpdate == nullptr) {
            return new ModuWrapBipolarToggleBox(config);
        }
        else {
            static_cast<ModuWrapBipolarToggleBox*>(existingComponentToUpdate)->SetModulationConfig(config);
            return existingComponentToUpdate;
        }
    }
    else if (columnId == 5) { // amount
        if (existingComponentToUpdate == nullptr) {
            return new ModuWrapAmountSlider(config);
        }
        else {
            static_cast<ModuWrapAmountSlider*>(existingComponentToUpdate)->SetModulationConfig(config);
            return existingComponentToUpdate;
        }
    }

    return nullptr;
}

void ModulationMatrixLayout::OnAddButtonClick() {
    auto modulator_id = modulators_.getText();
    auto parameter_id = parameters_.getText();
    juce::ScopedLock lock{processor_.getCallbackLock()};
    synth_.modulation_matrix.AddModulationByName(modulator_id, parameter_id);
}

void ModulationMatrixLayout::timerCallback() {
    if (!synth_.modulation_matrix.changed.exchange(false)) {
        return;
    }
    local_modulations_ = synth_.modulation_matrix.GetDoingModulations();
    table_->updateContent();
    repaint();
}

void ModulationMatrixLayout::cellClicked(int rowNumber, int columnId, const juce::MouseEvent& mouseEvent) {
    if (rowNumber >= static_cast<int>(local_modulations_.size())) return;
    if (mouseEvent.mods.isPopupMenu() && columnId == 1) {
        juce::PopupMenu popup;
        popup.addItem("remove", [ptr_mod = local_modulations_[size_t(rowNumber)], this] {
            juce::ScopedLock lock{processor_.getCallbackLock()};
            synth_.modulation_matrix.Remove(ptr_mod);
        });

        juce::PopupMenu source_popup;
        for (auto&[name, ptr] : synth_.modulation_matrix.modulator_hash_table_) {
            source_popup.addItem(name, [ptr_mod = local_modulations_[size_t(rowNumber)], ptr_source = ptr, this] {
                juce::ScopedLock lock{processor_.getCallbackLock()};
                synth_.modulation_matrix.ChangeSource(ptr_mod, ptr_source);
            });
        }
        popup.addSubMenu("replace source", source_popup);

        popup.showMenuAsync(juce::PopupMenu::Options{}.withMousePosition());
    }
    else if (mouseEvent.mods.isPopupMenu() && columnId == 2) {
        juce::PopupMenu popup;
        popup.addItem("remove", [ptr_mod = local_modulations_[size_t(rowNumber)], this] {
            juce::ScopedLock lock{processor_.getCallbackLock()};
            synth_.modulation_matrix.Remove(ptr_mod);
        });

        juce::PopupMenu source_popup;
        for (auto&[name, ptr] : synth_.modulation_matrix.parameter_hash_table_) {
            source_popup.addItem(name, [ptr_mod = local_modulations_[size_t(rowNumber)], ptr_target = ptr, this] {
                juce::ScopedLock lock{processor_.getCallbackLock()};
                synth_.modulation_matrix.ChangeTarget(ptr_mod, ptr_target);
            });
        }
        popup.addSubMenu("replace target", source_popup);

        popup.showMenuAsync(juce::PopupMenu::Options{}.withMousePosition());
    }
}
}
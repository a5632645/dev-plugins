#include "modulations.hpp"

#include <pluginshared/component.hpp>

#include "../dsp/synth.hpp"
#include "../PluginProcessor.h"

namespace analogsynth {
// -------------------- amount slider --------------------
class ModuWrapAmountSlider : public juce::Component {
public:
    ModuWrapAmountSlider(ModulationInfo* cfg) {
        amount_.slider.setRange(-1.0f, 1.0f);
        amount_.slider.setDoubleClickReturnValue(true, 0.0);
        amount_.slider.onValueChange = [this]() {
            if (config_ == nullptr) return;
            config_->amount = static_cast<float>(amount_.slider.getValue());
            UpdateLabelText();
        };
        amount_.SetTitleLayout(ui::FlatSlider::TitleLayout::None);
        addAndMakeVisible(amount_);

        begin_value_.setJustificationType(juce::Justification::centredLeft);
        // begin_value_.setEditable(false, true);
        addAndMakeVisible(begin_value_);
        // end_value_.setEditable(false, true);
        end_value_.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(end_value_);

        SetModulationConfig(cfg);
    }

    ~ModuWrapAmountSlider() override {
        if (config_ != nullptr) {
            config_->changed = []{};
        }
    }

    void UpdateLabelText() {
        if (config_ == nullptr) return;

        float value01 = static_cast<juce::RangedAudioParameter*>(config_->target->ptr_)->getValue();
        float min_value = value01 + config_->ConvertFrom01(0);
        float max_value = value01 + config_->ConvertFrom01(1);
        min_value = std::clamp(min_value, 0.0f, 1.0f);
        max_value = std::clamp(max_value, 0.0f, 1.0f);
        auto* text_ptr = static_cast<juce::RangedAudioParameter*>(config_->target->ptr_);
        begin_value_.setText(text_ptr->getText(min_value, 1024), juce::dontSendNotification);
        end_value_.setText(text_ptr->getText(max_value, 1024), juce::dontSendNotification);
    }

    void SetModulationConfig(ModulationInfo* cfg) {
        if (config_ != nullptr) {
            config_->changed = []{};
        }

        config_ = cfg;
        amount_.slider.setValue(cfg->amount, juce::dontSendNotification);
        amount_.slider.onValueChange();

        if (config_ != nullptr) {
            config_->changed = [this] {
                UpdateLabelText();
            };
        }
    }

    void resized() override {
        auto b = getLocalBounds();
        begin_value_.setBounds(b.removeFromLeft(60));
        end_value_.setBounds(b.removeFromRight(60));
        amount_.setBounds(b.reduced(1));
    }
private:
    ui::FlatSlider amount_;
    juce::Label begin_value_;
    juce::Label end_value_;
    ModulationInfo* config_{};
};

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
        config_ = cfg;
        setToggleState(cfg->enable, juce::dontSendNotification);
    }
private:
    ModulationInfo* config_{};

    // 通过 Listener 继承
    void buttonClicked(Button*) override {
        if (config_ == nullptr) return;
        config_->enable = getToggleState();
    }
};

// -------------------- toggle box2 --------------------
class ModuWrapBipolarToggleBox : public juce::ToggleButton {
public:
    ModuWrapBipolarToggleBox(ModulationInfo* cfg) {
        this->onClick = [this]() {
            if (config_ == nullptr) return;
            config_->bipolar = getToggleState();
            if (config_->changed) config_->changed();
        };
        SetModulationConfig(cfg);
    }

    void SetModulationConfig(ModulationInfo* cfg) {
        setToggleState(cfg->bipolar, juce::dontSendNotification);
        config_ = cfg;
    }
private:
    ModulationInfo* config_{};
};

// ----------------------------------------
// modulation gui
// ----------------------------------------
ModulationMatrixLayout::ModulationMatrixLayout(AnalogSynthAudioProcessor& p)
    : processor_(p)
    , synth_(p.synth_) {
    table_ = std::make_unique<juce::TableListBox>("table", this);
    int flags_NoResize = juce::TableHeaderComponent::visible;
    table_->getHeader().addColumn("modulator", 1, 100, 30, -1, flags_NoResize);
    table_->getHeader().addColumn("param", 2, 120, 30, -1, flags_NoResize);
    table_->getHeader().addColumn("enable", 3, 30, 30, -1, flags_NoResize);
    table_->getHeader().addColumn("bipolar", 4, 30, 30, -1, flags_NoResize);
    table_->getHeader().addColumn("amount", 5, 250, 30, -1, flags_NoResize);
    addAndMakeVisible(*table_);

    for (int item_id = 1; auto&[a,b] : synth_.modulation_matrix.modulator_hash_table_) {
        modulator_names_.push_back(a);
        modulator_popup_.addItem(item_id, a);
        ++item_id;
    }

    std::unordered_map<juce::String, std::vector<std::pair<juce::String, juce::String>>> parameter_classicfy;
    for (auto&[a,b] : synth_.modulation_matrix.parameter_hash_table_) {
        int idx = a.indexOfAnyOf("_.");
        if (idx == -1) {
            parameter_classicfy["misc"].push_back({a, a});
        }
        else {
            parameter_classicfy[a.substring(0, idx)].push_back({a, a.substring(idx + 1)});
        }
    }
    int item_id = 1;
    for (auto&[classicfy, names] : parameter_classicfy) {
        juce::PopupMenu sub;
        for (auto&[full, name] : names) {
            sub.addItem(item_id, name);
            parameter_names_.push_back(full);
            ++item_id;
        }
        parameter_popup_.addSubMenu(classicfy, sub);
    }

    startTimerHz(10);
}

ModulationMatrixLayout::~ModulationMatrixLayout() {
    table_->setModel(nullptr);
    table_ = nullptr;
}

void ModulationMatrixLayout::resized() {
    auto b = getLocalBounds();
    table_->setBounds(b);
}

int ModulationMatrixLayout::getNumRows() {
    return static_cast<int>(std::min(local_modulations_.size() + 1, synth_.modulation_matrix.kMaxModulations));
}

void ModulationMatrixLayout::paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) {
    juce::ignoreUnused(rowNumber, width, height);
    if (rowIsSelected) {
        g.setColour(ui::active_bg);
        g.drawRect(g.getClipBounds());
    }
}

void ModulationMatrixLayout::paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) {
    std::ignore = rowIsSelected;
    if (columnId != 1 && columnId != 2) {
        return;
    }
    if (rowNumber >= getNumRows()) {
        return;
    }

    // 一个预备位置
    g.setColour(juce::Colours::white);
    if (rowNumber == static_cast<int>(local_modulations_.size())) {
        if (columnId == 1) {
            g.drawText(pending_modulator_name_.isEmpty() ? "---" : pending_modulator_name_, 2, 0, width - 4, height, juce::Justification::centredLeft, true);
        }
        else if (columnId == 2) {
            g.drawText(pending_parameter_name_.isEmpty() ? "---" : pending_parameter_name_, 2, 0, width - 4, height, juce::Justification::centredLeft, true);
        }
        return;
    }

    auto config = local_modulations_[size_t(rowNumber)];
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

    if (rowNumber == static_cast<int>(local_modulations_.size())) {
        if (existingComponentToUpdate != nullptr) {
            delete existingComponentToUpdate;
        }
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
            auto* bipolar = static_cast<ModuWrapBipolarToggleBox*>(existingComponentToUpdate);
            bipolar->SetModulationConfig(config);
            return existingComponentToUpdate;
        }
    }
    else if (columnId == 5) { // amount
        if (existingComponentToUpdate == nullptr) {
            auto* slider = new ModuWrapAmountSlider(config);
            return slider;
        }
        else {
            auto* slider = static_cast<ModuWrapAmountSlider*>(existingComponentToUpdate);
            slider->SetModulationConfig(config);
            return existingComponentToUpdate;
        }
    }

    return nullptr;
}

void ModulationMatrixLayout::timerCallback() {
    if (!synth_.modulation_matrix.changed.exchange(false)) {
        return;
    }
    local_modulations_ = synth_.modulation_matrix.GetDoingModulations();
    table_->updateContent();
    repaint();
}

void ModulationMatrixLayout::CheckPendingAdd() {
    if (pending_modulator_name_.isEmpty() || pending_parameter_name_.isEmpty()) return;
    juce::ScopedLock lock{processor_.getCallbackLock()};
    auto[ptr, added] = synth_.modulation_matrix.AddModulationByName(pending_modulator_name_, pending_parameter_name_);
    if (added) {
        pending_modulator_name_.clear();
        pending_parameter_name_.clear();
    }
}

void ModulationMatrixLayout::cellClicked(int rowNumber, int columnId, const juce::MouseEvent& mouseEvent) {
    if (rowNumber >= getNumRows() || !mouseEvent.mods.isPopupMenu()) return;

    if (rowNumber == static_cast<int>(local_modulations_.size())) {
        if (columnId == 1) {
            modulator_popup_.showMenuAsync(juce::PopupMenu::Options{}.withMousePosition(),
            [this](int result_id) {
                if (result_id >= 1) {
                    pending_modulator_name_ = modulator_names_[static_cast<size_t>(result_id - 1)];
                    CheckPendingAdd();
                    repaint();
                }
            });
        }
        else if (columnId == 2) {
            parameter_popup_.showMenuAsync(juce::PopupMenu::Options{}.withMousePosition(),
            [this](int result_id) {
                if (result_id >= 1) {
                    pending_parameter_name_ = parameter_names_[static_cast<size_t>(result_id - 1)];
                    CheckPendingAdd();
                    repaint();
                }
            });
        }
        return;
    }

    if (columnId == 1) {
        juce::PopupMenu popup;
        popup.addSubMenu("replace source", modulator_popup_);
        popup.addItem("remove", [ptr_mod = local_modulations_[size_t(rowNumber)], this] {
            juce::ScopedLock lock{processor_.getCallbackLock()};
            synth_.modulation_matrix.Remove(ptr_mod);
        });

        popup.showMenuAsync(juce::PopupMenu::Options{}.withMousePosition(),
        [ptr_mod = local_modulations_[size_t(rowNumber)], this](int result_id) {
            if (result_id >= 1) {
                juce::ScopedLock lock{processor_.getCallbackLock()};
                synth_.modulation_matrix.ChangeSource(ptr_mod, synth_.modulation_matrix.FindSource(modulator_names_[static_cast<size_t>(result_id - 1)]));
            }
        });
    }
    else if (columnId == 2) {
        juce::PopupMenu popup;
        popup.addSubMenu("replace target", parameter_popup_);
        popup.addItem("remove", [ptr_mod = local_modulations_[size_t(rowNumber)], this] {
            juce::ScopedLock lock{processor_.getCallbackLock()};
            synth_.modulation_matrix.Remove(ptr_mod);
        });

        popup.showMenuAsync(juce::PopupMenu::Options{}.withMousePosition(),
        [ptr_mod = local_modulations_[size_t(rowNumber)], this](int result_id) {
            if (result_id >= 1) {
                juce::ScopedLock lock{processor_.getCallbackLock()};
                synth_.modulation_matrix.ChangeTarget(ptr_mod, synth_.modulation_matrix.FindTarget(parameter_names_[static_cast<size_t>(result_id - 1)]));
            }
        });
    }
}
}
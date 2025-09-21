#pragma once
#include "juce_core/juce_core.h"
#include "juce_graphics/juce_graphics.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>

namespace ui {
class XYPad : juce::Component {
public:
    enum class Mode {
        XY,
        Polar
    };

    XYPad();
    ~XYPad() override;
    // XY: x->x Polar: gain->x
    void BindParamX(juce::AudioProcessorValueTreeState& apvts, const char* id);
    // XY: y->y Polar: phase->y
    void BindParamY(juce::AudioProcessorValueTreeState& apvts, const char* id);
    void SetMode(Mode mode);
    void paint(juce::Graphics& g) override;
    void EvalCirclePos();
private:
    void DrawXy(juce::Graphics& g);
    void DrawPolar(juce::Graphics& g);
    
    class Circle;
    friend class Circle;

    Mode mode_{Mode::XY};
    juce::Slider x_;
    juce::Slider y_;
    std::unique_ptr<juce::SliderParameterAttachment> attach_x_;
    std::unique_ptr<juce::SliderParameterAttachment> attach_y_;
    std::unique_ptr<Circle> circle_;
};
}
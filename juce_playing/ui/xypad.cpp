#include "xypad.hpp"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <cassert>
#include <memory>
#include <numbers>

namespace ui {
class XYPad::Circle : public juce::Component {
public:
    Circle(XYPad& parent)
        : parent_(parent)
    {
    }

    void paint(juce::Graphics& g) override {
        auto b = getLocalBounds();
        g.setColour(juce::Colours::darkgrey);
        g.fillEllipse(b.toFloat());
        g.setColour(juce::Colours::white);
        g.drawEllipse(b.toFloat(), 1.0f);
    }

    void mouseDown(const juce::MouseEvent& e) override {
        drag_.startDraggingComponent(this, e);
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        drag_.dragComponent(this, e, nullptr);
        auto b = getBounds();
        auto parent_b = parent_.getLocalBounds();
        auto center = b.getCentre();
        center.x = std::clamp(center.x, parent_b.getX(), parent_b.getRight());
        center.y = std::clamp(center.y, parent_b.getY(), parent_b.getBottom());
        setCentrePosition(center);
        float fx = static_cast<float>(center.x) / parent_b.getWidth();
        float fy = 1.0f - static_cast<float>(center.y) / parent_b.getHeight();
        if (parent_.mode_ == Mode::XY) {
            fx = std::clamp(fx, 0.0f, 1.0f);
            fy = std::clamp(fy, 0.0f, 1.0f);
            parent_.x_.setValue(parent_.x_.getNormalisableRange().convertFrom0to1(fx));
            parent_.y_.setValue(parent_.y_.getNormalisableRange().convertFrom0to1(fy));
        }
        else {
            fx = 2 * fx - 1;
            fy = 2 * fy - 1;
            float g = std::sqrt(fx * fx + fy * fy);
            auto arg = std::atan2(fy, fx);
            if (arg < 0) {
                arg += std::numbers::pi_v<float> * 2;
            }
            arg /= std::numbers::pi_v<float> * 2;
            g = std::clamp(g, 0.0f, 1.0f);
            arg = std::clamp(arg, 0.0f, 1.0f);
            parent_.x_.setValue(parent_.x_.getNormalisableRange().convertFrom0to1(g));
            parent_.y_.setValue(parent_.y_.getNormalisableRange().convertFrom0to1(arg));
        }
    }
private:
    XYPad& parent_;
    juce::ComponentDragger drag_;
};

XYPad::XYPad() {
    circle_ = std::make_unique<Circle>(*this);
    addAndMakeVisible(*circle_);

    x_.onValueChange = [this] {
        EvalCirclePos();
    };
    y_.onValueChange = [this] {
        EvalCirclePos();
    };
}

XYPad::~XYPad() = default;

void XYPad::BindParamX(juce::AudioProcessorValueTreeState& apvts, const char* id) {
    auto* p = apvts.getParameter(juce::String::fromUTF8(id));
    jassert(p != nullptr);
    attach_x_ = std::make_unique<juce::SliderParameterAttachment>(*p, x_);
}

void XYPad::BindParamY(juce::AudioProcessorValueTreeState& apvts, const char* id) {
    auto* p = apvts.getParameter(juce::String::fromUTF8(id));
    jassert(p != nullptr);
    attach_y_ = std::make_unique<juce::SliderParameterAttachment>(*p, y_);
}

void XYPad::SetMode(Mode mode) {
    // TODO: fix polar not working
    assert(mode != Mode::Polar);
    mode_ = mode;
    repaint();
    EvalCirclePos();
}

void XYPad::EvalCirclePos() {
    if (mode_ == Mode::XY) {
        float fx = x_.getNormalisableRange().convertTo0to1(x_.getValue());
        float fy = y_.getNormalisableRange().convertTo0to1(y_.getValue());
        circle_->setCentrePosition(fx * getWidth(), (1.0f - fy) * getHeight());
    }
    else {
        float g = x_.getNormalisableRange().convertTo0to1(x_.getValue());
        float phase = y_.getNormalisableRange().convertTo0to1(y_.getValue()) * std::numbers::pi_v<float> * 2.0f;
        circle_->setCentrePosition(g * (std::cos(phase) * 0.5f + 0.5f) * getWidth(), (1.0f - g * (std::sin(phase) * 0.5f + 0.5f)) * getHeight());
    }
}

void XYPad::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::darkgrey);
    g.setColour(juce::Colours::white);
    g.drawRect(getLocalBounds());
    if (mode_ == Mode::XY) {
        DrawXy(g);
    }
    else if (mode_ == Mode::Polar) {
        DrawPolar(g);
    }
}

void XYPad::resized() {
    auto b = getLocalBounds();
    auto w = std::min(b.getHeight(), b.getWidth());
    auto cw = std::max(w * 0.01f, 16.0f);
    circle_->setSize(cw, cw);
}

void XYPad::DrawXy(juce::Graphics& g) {
    auto b = getLocalBounds();
    g.setColour(juce::Colours::white);
    g.drawVerticalLine(b.getCentreX(), 0, getHeight());
    g.drawHorizontalLine(b.getCentreY(), 0, getWidth());
}

void XYPad::DrawPolar(juce::Graphics& g) {
auto b = getLocalBounds();
    g.setColour(juce::Colours::white);
    g.drawVerticalLine(b.getCentreX(), 0, getHeight());
    g.drawHorizontalLine(b.getCentreY(), 0, getWidth());
    g.drawEllipse(b.toFloat(), 1.0f);
}
}
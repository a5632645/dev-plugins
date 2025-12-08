#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../dsp/curve_v2.h"

namespace mana {
class CommonCurveEditor;
}

namespace mana::detail { // why did the pimpl broken on my computer?
class CurvePowerPointComponent;
class CurveXYPointComponent;

class CurveXYPointComponent
    : public juce::Component {
public:
    CurveXYPointComponent(int idx);

    void paint(juce::Graphics& g) override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseEnter(const juce::MouseEvent& e) override { repaint(); }
    void mouseExit(const juce::MouseEvent& e) override { repaint(); }
    void mouseDoubleClick(const juce::MouseEvent& e) override;
private:
    friend class CommonCurveEditor;
    int idx_;
    std::unique_ptr<juce::PopupMenu> popup_menu_;
};

class CurvePowerPointComponent
    : public juce::Component {
public:
    CurvePowerPointComponent(int idx);

    void paint(juce::Graphics& g) override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseEnter(const juce::MouseEvent& /*e*/) override { repaint(); }
    void mouseExit(const juce::MouseEvent& /*e*/) override { repaint(); }
    void mouseDoubleClick(const juce::MouseEvent& e) override;
private:
    friend class CommonCurveEditor;
    int idx_;
    float last_power_{};
};
}

namespace mana {
class CommonCurveEditor
    : public juce::Component
    , public CurveV2::Listener {
public:
    ~CommonCurveEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void SetCurve(CurveV2* new_curve);
    void SetSnapGrid(bool snap) { snap_grid_ = snap; }
    void SetDisplayGrid(bool show) { display_grid_ = show; repaint(); }
    void SetGridNum(int x, int y) { x_grid_ = x; y_grid_ = y; repaint(); }
    juce::Rectangle<int> GetComponentBounds() const;
private:
    bool snap_grid_{ false };
    bool display_grid_{ true };
    int x_grid_{ 8 };
    int y_grid_{ 1 };

    void mouseDoubleClick(const juce::MouseEvent& e) override;

    friend class ::mana::detail::CurveXYPointComponent;
    friend class ::mana::detail::CurvePowerPointComponent;

    void DragXyPoint(detail::CurveXYPointComponent& p, const juce::MouseEvent& e);
    void SetXyPointPos(detail::CurveXYPointComponent& p);
    void SetXyPointPos(int idx) { SetXyPointPos(*xy_points_[idx]); }
    void RemoveXyPoint(detail::CurveXYPointComponent& p);
    bool IsFirstXyPoint(int idx) const { return idx == 0; }
    bool IsLastXyPoint(int idx) const { return idx == static_cast<int>(xy_points_.size() - 1); }

    void SetPowerPointPos(detail::CurvePowerPointComponent& p);
    void SetPowerPointPos(int idx) { SetPowerPointPos(*power_points_[idx]); }


    CurveV2* curve_{};
    std::vector<std::unique_ptr<detail::CurveXYPointComponent>> xy_points_;
    std::vector<std::unique_ptr<detail::CurvePowerPointComponent>> power_points_;

    // 通过 Listener 继承
    void OnAddPoint(CurveV2* generator, CurveV2::Point p, int before_idx) override;
    void OnRemovePoint(CurveV2* generator, int remove_idx) override;
    void OnReload(CurveV2* generator) override;
    void OnPointXyChanged(CurveV2* generator, int changed_idx) override;
    void OnPointPowerChanged(CurveV2* generator, int changed_idx) override;
};
}
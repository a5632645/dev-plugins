#include "common_curve_editor.h"

static constexpr auto width = 20;
static constexpr auto height = width;
static constexpr auto kMaxOfDistance = 200;
static constexpr auto kCompCircleSize = 5.0f;

namespace mana::detail {
CurveXYPointComponent::CurveXYPointComponent(int idx)
    : idx_(idx) {
    setSize(width, height);
}

void CurveXYPointComponent::paint(juce::Graphics& g) {
    if (isMouseOver()) {
        g.setColour(juce::Colours::white);
        g.drawEllipse(getLocalBounds().toFloat(), 1.0f);
    }
    g.setColour(juce::Colours::white);
    g.fillEllipse(juce::Rectangle{ 0.0f,0.0f,kCompCircleSize,kCompCircleSize }.withCentre(getLocalBounds().getCentre().toFloat()));
}

void CurveXYPointComponent::mouseDown(const juce::MouseEvent& e) {
    if (!e.mods.isRightButtonDown())
        return;

    auto* parent = static_cast<CommonCurveEditor*>(getParentComponent());
    if (parent->IsLastXyPoint(idx_))
        return;

    auto* curve = parent->curve_;
    using pe = CurveV2::PowerEnum;
    auto set_type = [idx = idx_, curve](pe type) {
        return [i = idx, curve, t = type]() {
            curve->SetPowerType(i, t);
        };
    };

    // will it dangling pointer/ref?
    popup_menu_ = std::make_unique<juce::PopupMenu>();
    auto curr_power_type = parent->curve_->GetPoint(idx_).power_type;
    popup_menu_->addItem("keep", true, curr_power_type == pe::kKeep, set_type(pe::kKeep));
    popup_menu_->addItem("exp", true, curr_power_type == pe::kExp, set_type(pe::kExp));
    popup_menu_->addItem("wave_sine", true, curr_power_type == pe::kWaveSine, set_type(pe::kWaveSine));
    popup_menu_->addItem("wave_tri", true, curr_power_type == pe::kWaveTri, set_type(pe::kWaveTri));
    popup_menu_->addItem("wave_square", true, curr_power_type == pe::kWaveSquare, set_type(pe::kWaveSquare));
    popup_menu_->addSeparator();
    popup_menu_->addItem("delete", [idx = idx_, curve] {curve->Remove(idx); });
    popup_menu_->showMenuAsync(juce::PopupMenu::Options{});
}

void CurveXYPointComponent::mouseDrag(const juce::MouseEvent& e) {
    static_cast<CommonCurveEditor*>(getParentComponent())->DragXyPoint(*this, e);
}

void CurveXYPointComponent::mouseDoubleClick(const juce::MouseEvent& e) {
    static_cast<CommonCurveEditor*>(getParentComponent())->RemoveXyPoint(*this);
}

CurvePowerPointComponent::CurvePowerPointComponent(int idx)
    : idx_(idx) {
    setSize(width, height);
}

void CurvePowerPointComponent::paint(juce::Graphics& g) {
    if (isMouseOver()) {
        g.setColour(juce::Colours::white);
        g.drawEllipse(getLocalBounds().toFloat(), 1.0f);
    }
    g.setColour(juce::Colours::white);
    g.drawEllipse(juce::Rectangle{ 0.0f,0.0f,kCompCircleSize,kCompCircleSize }.withCentre(getLocalBounds().getCentre().toFloat()), 1.0f);
}

void CurvePowerPointComponent::mouseDown(const juce::MouseEvent& e) {
    auto* parent = static_cast<CommonCurveEditor*>(getParentComponent());
    last_power_ = parent->curve_->GetPoint(idx_).power;
}

void CurvePowerPointComponent::mouseDrag(const juce::MouseEvent& e) {
    auto* parent = static_cast<CommonCurveEditor*>(getParentComponent());
    auto nor_y = static_cast<float>(e.getDistanceFromDragStartY()) / static_cast<float>(kMaxOfDistance);
    parent->curve_->SetPower(idx_, last_power_ + nor_y);
}

void CurvePowerPointComponent::mouseDoubleClick(const juce::MouseEvent& e) {
    // reset power to 0
    static_cast<CommonCurveEditor*>(getParentComponent())->curve_->SetPower(idx_, 0.0f);
}
}

namespace mana {
CommonCurveEditor::~CommonCurveEditor() {
    if (curve_ != nullptr)
        curve_->RemoveListener(this);
}

void CommonCurveEditor::paint(juce::Graphics& g) {
    auto bg_color = juce::Colours::darkgrey;
    g.setColour(bg_color);
    g.fillRect(GetComponentBounds());

    if (display_grid_) {
        if (x_grid_ > 2) {
            g.setColour(juce::Colours::black);
            auto width = GetComponentBounds().toFloat().getWidth() / (x_grid_ - 1.0f);
            for (int i = 1; i < x_grid_; ++i) {
                g.drawVerticalLine(width * i + GetComponentBounds().getX(), GetComponentBounds().getY(), GetComponentBounds().getBottom());
            }
        }
        if (y_grid_ > 2) {
            g.setColour(juce::Colours::black);
            auto height = GetComponentBounds().toFloat().getHeight() / (y_grid_ - 1.0f);
            for (int i = 1; i < y_grid_; ++i) {
                g.drawHorizontalLine(height * i + GetComponentBounds().getY(), GetComponentBounds().getX(), GetComponentBounds().getRight());
            }
        }
    }

    if (curve_ != nullptr) {
        const auto num_points = GetComponentBounds().getWidth();

        auto convert_xy = [bound = GetComponentBounds().toFloat()](float x, float y) {
            struct R {
                float x;
                float y;
            };
            return R{
                bound.getX() + bound.getWidth() * x,
                bound.getY() + bound.getHeight() * (1.0f - y)
            };
        };

        juce::Path p;
        p.preallocateSpace(num_points);
        {
            auto [x, y] = convert_xy(0, curve_->Get(0));
            p.startNewSubPath(x, y);
        }
        auto inv_range = 1.0f / (num_points - 1.0f);
        for (int i = 1; i < num_points; ++i) {
            auto nor_x = static_cast<float>(i) * inv_range;
            auto nor_y = curve_->GetNormalize(nor_x);
            auto [x, y] = convert_xy(nor_x, nor_y);
            p.lineTo(x, y);
        }

        g.setColour(juce::Colours::darkgreen);
        g.strokePath(p, juce::PathStrokeType{ 3.0f });
        g.setColour(juce::Colours::lightgreen);
        g.strokePath(p, juce::PathStrokeType{ 1.0f });
    }

    g.setColour(bg_color.darker());
    g.drawRect(GetComponentBounds().expanded(2.0f, 2.0f), 2.0f);
}

void CommonCurveEditor::resized() {
    for (const auto& xy : xy_points_) {
        SetXyPointPos(*xy);
    }
    for (const auto& power : power_points_) {
        SetPowerPointPos(*power);
    }
}

void CommonCurveEditor::SetCurve(CurveV2* new_curve) {
    jassert(new_curve != nullptr);

    if (new_curve == curve_)
        return;

    if (curve_ != nullptr)
        curve_->RemoveListener(this);

    curve_ = new_curve;
    curve_->AddListener(this);
    OnReload(curve_);
}

void CommonCurveEditor::mouseDoubleClick(const juce::MouseEvent& e) {
    if (curve_ == nullptr)
        return;

    // todo: solve upside down in pow power mode
    auto nor_x = (e.getMouseDownX() - GetComponentBounds().getX()) / static_cast<float>(GetComponentBounds().getWidth());
    auto nor_y = 1.0f - (e.getMouseDownY() - GetComponentBounds().getY()) / static_cast<float>(GetComponentBounds().getHeight());
    nor_x = std::clamp(nor_x, 0.0f, 1.0f);
    nor_y = std::clamp(nor_y, 0.0f, 1.0f);
    curve_->AddPoint(CurveV2::Point{ nor_x, nor_y });
}

void CommonCurveEditor::DragXyPoint(detail::CurveXYPointComponent& p, const juce::MouseEvent& e) {
    if (curve_ == nullptr)
        return;

    auto ee = e.getEventRelativeTo(this);
    auto point_pos = ee.getPosition().toFloat();
    auto bound = GetComponentBounds().toFloat();
    auto nor_x = (point_pos.x - bound.getX()) / bound.getWidth();
    auto nor_y = 1.0f - (point_pos.y - bound.getY()) / bound.getHeight();

    nor_x = std::clamp(nor_x, 0.0f, 1.0f);
    nor_y = std::clamp(nor_y, 0.0f, 1.0f);
    if (snap_grid_) {
        if (x_grid_ > 2) {
            auto tx = (x_grid_ - 1.0f) * nor_x;
            auto near = std::round(tx);
            if (std::abs(near - tx) < 0.1f) {
                tx = near;
            }
            nor_x = tx / (x_grid_ - 1.0f);
        }
        if (y_grid_ > 2) {
            auto ty = (y_grid_ - 1.0f) * nor_y;
            auto near = std::round(ty);
            if (std::abs(near - ty) < 0.1f) {
                ty = near;
            }
            nor_y = ty / (y_grid_ - 1.0f);
        }
    }
    curve_->SetXy(p.idx_, nor_x, nor_y);
}

void CommonCurveEditor::SetPowerPointPos(detail::CurvePowerPointComponent& p) {
    auto before_center = xy_points_[p.idx_]->getBounds().getCentre().toFloat();
    auto after_center = xy_points_[p.idx_ + 1]->getBounds().getCentre().toFloat();
    auto bound = GetComponentBounds().toFloat();

    p.setVisible(before_center.x != after_center.x && before_center.y != after_center.y);

    auto x = std::midpoint(before_center.x, after_center.x);
    auto nor_x = (x - bound.getX()) / bound.getWidth();
    auto y = std::lerp(bound.getBottom(), bound.getY(), curve_->GetNormalize(nor_x));
    //auto y = std::midpoint(before_center.y, after_center.y);
    //p.setBounds(p.getBounds().withCentre(juce::Point{ x,y }));
    p.setBounds(p.getBounds().withCentre(juce::Point{ x,y, }.roundToInt()));
}

void CommonCurveEditor::SetXyPointPos(detail::CurveXYPointComponent& p) {
    auto point = curve_->GetPoint(p.idx_);
    auto x = GetComponentBounds().getWidth() * point.x + GetComponentBounds().getX();
    auto y = GetComponentBounds().getHeight() * (1.0f - point.y) + GetComponentBounds().getY();
    p.setBounds(p.getBounds().toFloat().withCentre(juce::Point{ x,y }).toNearestInt());
}

void CommonCurveEditor::RemoveXyPoint(detail::CurveXYPointComponent& p) {
    if (IsFirstXyPoint(p.idx_) || IsLastXyPoint(p.idx_))
        return;

    curve_->Remove(p.idx_);
}

juce::Rectangle<int> CommonCurveEditor::GetComponentBounds() const {
    return getLocalBounds().reduced(width / 2, height / 2);
}

void CommonCurveEditor::OnAddPoint(CurveV2* generator, CurveV2::Point point, int before_idx) {
    if (curve_ != generator)
        return;

    auto add_idx = before_idx + 1;
    xy_points_.emplace(xy_points_.cbegin() + add_idx, std::make_unique<detail::CurveXYPointComponent>(add_idx));
    power_points_.emplace(power_points_.cbegin() + add_idx, std::make_unique<detail::CurvePowerPointComponent>(add_idx));

    for (int i = add_idx + 1; i < xy_points_.size(); ++i)
        xy_points_[i]->idx_++;
    for (int i = add_idx + 1; i < power_points_.size(); ++i)
        power_points_[i]->idx_++;

    const auto& add_xy = xy_points_[add_idx];
    const auto& add_power = power_points_[add_idx];
    const auto& before_power = power_points_[before_idx];

    // todo: show new component and adjust addxy,before_power,add_power bounds
    addAndMakeVisible(add_xy.get());
    addAndMakeVisible(add_power.get());
    SetXyPointPos(*add_xy);
    //LimitXyPoint(*add_xy);
    SetPowerPointPos(*add_power);
    SetPowerPointPos(*before_power);
    repaint();
}

void CommonCurveEditor::OnRemovePoint(CurveV2* generator, int idx) {
    if (curve_ != generator)
        return;

    xy_points_.erase(xy_points_.cbegin() + idx);
    power_points_.erase(power_points_.cbegin() + idx);

    for (int i = idx; i < xy_points_.size(); ++i)
        xy_points_[i]->idx_--;
    for (int i = idx; i < power_points_.size(); ++i)
        power_points_[i]->idx_--;

    // todo: adjust before_power bounds
    SetPowerPointPos(*power_points_[idx - 1]);
    repaint();
}

void CommonCurveEditor::OnReload(CurveV2* generator) {
    if (curve_ != generator)
        return;

    // rebuild interface
    power_points_.clear();
    xy_points_.clear();

    const auto& points = curve_->GetAllPoints();
    auto num_points = static_cast<int>(points.size());
    for (int i = 0; const auto & p : points) {
        xy_points_.emplace_back(std::make_unique<detail::CurveXYPointComponent>(i++));
    }
    for (int i = 0; i < num_points - 1; ++i) {
        power_points_.emplace_back(std::make_unique<detail::CurvePowerPointComponent>(i));
    }

    for (const auto& xy : xy_points_)
        addAndMakeVisible(xy.get());
    for (const auto& power : power_points_)
        addAndMakeVisible(power.get());

    resized();
    repaint();
}
void CommonCurveEditor::OnPointXyChanged(CurveV2* generator, int changed_idx) {
    if (IsFirstXyPoint(changed_idx)) {
        SetXyPointPos(changed_idx);
        SetPowerPointPos(changed_idx);
    }
    else if (IsLastXyPoint(changed_idx)) {
        SetXyPointPos(changed_idx);
        SetPowerPointPos(changed_idx - 1);
    }
    else {
        SetXyPointPos(changed_idx);
        SetPowerPointPos(changed_idx - 1);
        SetPowerPointPos(changed_idx);
    }
    repaint();
}

void CommonCurveEditor::OnPointPowerChanged(CurveV2* generator, int changed_idx) {
    SetXyPointPos(changed_idx);
    SetXyPointPos(changed_idx + 1);
    SetPowerPointPos(changed_idx);
    repaint();
}
}
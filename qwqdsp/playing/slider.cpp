#include <algorithm>
#include <cassert>
#include <cmath>
#include <numbers>
#include <string>
#include "slider.hpp"
#include "raylib.h"

inline static Knob* s_currentKnob = nullptr;

void Knob::display() {
    if (!enable_) {
        if (s_currentKnob == this) {
            s_currentKnob = nullptr;
        }
        return;
    }

    // logic
    auto const mousePosition = GetMousePosition();
    bool const isNowDown = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
    if (isNowDown && m_isPressed && s_currentKnob == this) {
        auto scale = 1.0f;
        if (IsKeyDown(KEY_LEFT_SHIFT)) {
            scale *= 0.1f;
        }

        if (IsKeyDown(KEY_LEFT_ALT)) {
            scale *= 0.1f;
        }

        auto const deltaY = m_lastMousePosition.y - mousePosition.y;
        m_counter += static_cast<int>(deltaY);
        auto const valueInc = m_counter / m_sensitivity;
        m_counter %= m_sensitivity;
        m_value += valueInc * m_step * scale;
        m_lastMousePosition = mousePosition;
        auto newValue = std::clamp(m_value, m_min, m_max);
        m_value = newValue;

        on_value_change(m_value);
    }
    else if (isNowDown && !m_isPressed) {
        if (s_currentKnob == nullptr) {
            if (CheckCollisionPointRec(mousePosition, m_bounds)) {
                m_counter = 0;
                m_isPressed = true;
                m_lastMousePosition = mousePosition;
                s_currentKnob = this;
            }
        }
    }
    else if (!isNowDown && m_isPressed && s_currentKnob == this) {
        m_counter = 0;
        m_isPressed = false;
        s_currentKnob = nullptr;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && CheckCollisionPointRec(mousePosition, m_bounds)) {
        // set to default
        m_value = m_default_value;
        m_counter = 0;
        m_isPressed = false;
        on_value_change(m_value);
    }

    // draw name
    {
        auto const numberTextWidth = MeasureText(m_name.data(), m_number_font_size);
        auto const exSpace = m_bounds.width - numberTextWidth;
        DrawText(m_name.data(), m_bounds.x + exSpace * 0.5F, m_bounds.y, m_name_font_size, m_fore_color);
    }

    Rectangle slider = {
        .x = m_bounds.x,
        .y = m_bounds.y + m_name_font_size,
        .width = m_bounds.width,
        .height = m_bounds.height - m_name_font_size - m_number_font_size
    };

    Rectangle text_bound = {
        .x = m_bounds.x,
        .y = m_bounds.y + m_bounds.height - m_number_font_size,
        .width = m_bounds.width,
        .height = static_cast<float>(m_number_font_size)
    };

    // draw number
    auto num_val = m_value;
    auto const numberText = value_to_text_function(num_val);
    auto const numberTextWidth = MeasureText(numberText.c_str(), m_number_font_size);
    auto const exSpace = text_bound.width - numberTextWidth;
    DrawText(numberText.c_str(), m_bounds.x + exSpace * 0.5F, text_bound.y, m_number_font_size, m_fore_color);

    // draw circle
    auto const centerX = slider.x + slider.width / 2.0F;
    auto const centerY = slider.y + slider.height / 2.0F;
    auto const radius = std::min(slider.width, slider.height) / 2.0F;
    DrawCircle(centerX, centerY, radius, m_bg_color);
    DrawCircleLines(centerX, centerY, radius, m_fore_color);

    // draw pointer
    {
        auto const pw = (m_value - m_min) / (m_max - m_min);
        auto const startAngle = 5.0F / 4.0F * std::numbers::pi_v<float>;
        auto const endAngle = -1.0F / 4.0F * std::numbers::pi_v<float>;
        auto const angle = startAngle + pw * (endAngle - startAngle);
        Vector2 const pointerPos = {
        .x = centerX + radius * std::cos(angle),
        .y = centerY - radius * std::sin(angle)
        };
        DrawLine(centerX, centerY, pointerPos.x, pointerPos.y, m_fore_color);
    }
}

Knob& Knob::set_title(std::string_view name) {
    m_name = name;
    return *this;
}

Knob& Knob::set_range(float min, float max, float step, float default_value) {
    m_min = min;
    m_max = max;
    m_step = step;
    m_default_value = default_value;
    if (m_value != default_value) {
        m_value = default_value;
        on_value_change(m_value);
    }

    auto num = (m_max - m_min) / m_step;
    if (num > 400) {
        m_sensitivity = 1;
    }
    else if (num > 100) {
        m_sensitivity = 2;
    }
    else if (num > 80) {
        m_sensitivity = 3;
    }
    else if (num > 50) {
        m_sensitivity = 4;
    }
    else if (num > 20) {
        m_sensitivity = 8;
    }
    else {
        m_sensitivity = 50;
    }
    return *this;
}

Knob& Knob::SetDefaultValue(float dv) {
    m_default_value = dv;
    return *this;
}

Knob& Knob::set_sensitivity(int sensitivity) {
    m_sensitivity = sensitivity;
    return *this;
}

Knob& Knob::set_fore_color(Color fore_color) {
    m_fore_color = fore_color;
    return *this;
}

Knob& Knob::set_bg_color(Color bg_color) {
    m_bg_color = bg_color;
    return *this;
}

Knob& Knob::set_name_font_size(int name_font_size) {
    m_name_font_size = name_font_size;
    return *this;
}

Knob& Knob::set_number_font_size(int number_font_size) {
    m_number_font_size = number_font_size;
    return *this;
}

Knob& Knob::set_value(float v) {
    m_value = std::clamp(v, m_min, m_max);
    return *this;
}

Knob& Knob::set_bound(Rectangle bound) {
    m_bounds = bound;
    return *this;
}

Knob& Knob::set_bound(int x, int y, int w, int h) {
    m_bounds.x = x;
    m_bounds.y = y;
    m_bounds.width = w;
    m_bounds.height = h;
    return *this;
}

float Knob::get_value() const {
    return m_value;
}

void Knob::SetEnable(bool enable) {
    enable_ = enable;
}

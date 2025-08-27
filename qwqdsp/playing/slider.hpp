#pragma once

#include "raylib.h"
#include <string_view>
#include <functional>
#include <string>

class Knob {
public:
    void display();

    Knob& set_title(std::string_view name);
    Knob& set_range(float min, float max, float step, float default_value);
    Knob& SetDefaultValue(float dv);
    Knob& set_sensitivity(int sensitivity);
    Knob& set_fore_color(Color fore_color);
    Knob& set_bg_color(Color bg_color);
    Knob& set_name_font_size(int name_font_size);
    Knob& set_number_font_size(int number_font_size);
    Knob& set_value(float v);
    Knob& set_bound(Rectangle bound);
    Knob& set_bound(int x, int y, int w, int h);

    float get_value() const;

    void SetEnable(bool enable);
    
    std::function<std::string(float)> value_to_text_function = EmptyV2Tcaller;
    std::function<void(float)> on_value_change = [](float){};
protected:
    std::string_view m_name{ "unkown" };
    float m_default_value{};
    float m_value{};
    float m_min{};
    float m_max{};
    float m_step{};
    int m_sensitivity = 2;
    Color m_fore_color = WHITE;
    Color m_bg_color = BLACK;
    int m_name_font_size = 10;
    int m_number_font_size = 10;

    bool m_isPressed = false;
    bool enable_ = true;
    Vector2 m_lastMousePosition{};
    int m_counter = 0;

    Rectangle m_bounds{};
private:
    static void empty_callback(float) {}
    static std::string EmptyV2Tcaller(float e) { return std::to_string(e); }
};

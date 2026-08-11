#pragma once
#include <array>
#include <cmath>
#include <string>

#include <juce_graphics/juce_graphics.h>
#include <pluginshared/color_preset.hpp>

namespace green_vocoder::ui {

// ------------------------------------------------------------
// 频谱曲线绘制公共部分（burg / stft 共用）
// ------------------------------------------------------------

// 填充曲线绘制区背景
inline void FillPlotBackground(juce::Graphics& g, juce::Rectangle<float> bb) {
    g.setColour(::ui::black_bg);
    g.fillRect(bb);
}

// db → y 坐标；低于 bound_bottom_db 返回 false（不绘制、不 clamp 到 floor），
// 高于 bound_top_db 时 clamp 到顶部。
inline bool DbToY(float db, float bound_top_db, float bound_bottom_db, juce::Rectangle<float> bb, float& y) {
    if (db < bound_bottom_db)
        return false;
    if (db > bound_top_db)
        db = bound_top_db;
    float const nor = (db - bound_bottom_db) / (bound_top_db - bound_bottom_db);
    y = bb.getY() + bb.getHeight() * (1.0f - nor);
    return true;
}

// 水平 db 网格线与左侧刻度
inline void DrawDbGrid(juce::Graphics& g, juce::Rectangle<float> bb, int nlines, float top_line_db, float last_line_db,
                       float bound_top_db, float bound_bottom_db) {
    g.setColour(::ui::grid_fore);
    float const font_half = g.getCurrentFont().getHeight() * 0.5f;
    float const db_span = (top_line_db - last_line_db) / static_cast<float>(nlines - 1);
    for (int i = 0; i < nlines; ++i) {
        float const db = last_line_db + db_span * static_cast<float>(i);
        float y;
        if (!DbToY(db, bound_top_db, bound_bottom_db, bb, y))
            continue;
        g.drawHorizontalLine(static_cast<int>(y), bb.getX(), bb.getRight());
        g.drawSingleLineText(std::to_string(static_cast<int>(db)), static_cast<int>(bb.getX()),
                             static_cast<int>(y + font_half));
    }
}

// 对数频率网格与刻度（20Hz ~ 20kHz）
inline void DrawFreqGrid(juce::Graphics& g, juce::Rectangle<float> bb) {
    static const std::array kLogJtable{
        0.0f,
        std::log10(2.0f),
        std::log10(3.0f),
        std::log10(4.0f),
        std::log10(5.0f),
        std::log10(6.0f),
        std::log10(7.0f),
        std::log10(8.0f),
        std::log10(9.0f),
    };
    static const juce::StringArray kFreqStr{"20", "200", "2k", "20k"};
    auto const current_font = g.getCurrentFont();
    float const span_w = bb.getWidth() / 3.0f;
    for (int i = 0; i < 3; ++i) {
        float const span_x = span_w * static_cast<float>(i) + bb.getX();
        for (int j = 0; j < 9; ++j) {
            float const x = span_x + span_w * kLogJtable[static_cast<size_t>(j)];
            g.drawVerticalLine(static_cast<int>(x), bb.getY(), bb.getBottom());
        }
        if (i == 0) {
            g.drawSingleLineText(kFreqStr[i], static_cast<int>(span_x),
                                 static_cast<int>(bb.getBottom()) - static_cast<int>(current_font.getHeight() / 2));
        }
        else {
            auto const str_w = juce::TextLayout::getStringWidth(current_font, kFreqStr[i]);
            g.drawSingleLineText(kFreqStr[i], static_cast<int>(span_x - str_w / 2),
                                 static_cast<int>(bb.getBottom()) - static_cast<int>(current_font.getHeight() / 2));
        }
    }
    auto const last_w = juce::TextLayout::getStringWidth(current_font, kFreqStr[3]);
    g.drawSingleLineText(kFreqStr[3], static_cast<int>(bb.getRight()) - static_cast<int>(last_w),
                         static_cast<int>(bb.getBottom()) - static_cast<int>(current_font.getHeight() / 2));
}

// 遍历每个像素构建 db 曲线 Path 后一次性描边（避免逐像素 drawLine）；
// value_at(omega) 返回该点 db 值。起始点取自第一个有效点（不再固定）；
// 低于 floor 或 NaN 时断开曲线（新子路径）不绘制。
template <typename ValueFn>
void DrawDbCurve(juce::Graphics& g, juce::Rectangle<float> bb, float omega_base, float bound_top_db,
                 float bound_bottom_db, float freq_pow, ValueFn&& value_at) {
    juce::Path path;
    float const mul_val = std::pow(10.0f, freq_pow / bb.getWidth());
    float mul_begin = 1.0f;
    bool have_last = false;
    float const x0 = bb.getX();
    for (int x = 0; x < static_cast<int>(bb.getWidth()); ++x) {
        float const omega = omega_base * mul_begin;
        mul_begin *= mul_val;
        float const db = value_at(omega);
        if (std::isnan(db)) {
            have_last = false;
            continue;
        }
        float y;
        if (!DbToY(db, bound_top_db, bound_bottom_db, bb, y)) {
            have_last = false; // 低于 floor：不绘制并断开曲线
            continue;
        }
        juce::Point<float> const p{x0 + static_cast<float>(x), y};
        if (have_last)
            path.lineTo(p);
        else
            path.startNewSubPath(p);
        have_last = true;
    }
    g.setColour(::ui::line_fore);
    g.strokePath(path, juce::PathStrokeType(2.0f));
}

} // namespace green_vocoder::ui

#include <algorithm>
#include <array>
#include <cstddef>
#include <span>

#include "raylib.h"
#include "../playing/slider.hpp"

#include "qwqdsp/convert.hpp"
#include "qwqdsp/filter/formant.hpp"
#include "qwqdsp/filter/biquad.hpp"
#include "qwqdsp/filter/rbj.hpp"
#include "qwqdsp/osciilor/dsf.hpp"

static qwqdsp::Biquad filters[5];
static std::array<float, 5> gains{};
static qwqdsp::oscillor::DSFClassic<13> dsf;
static float output_gain{1.0f};
static float q_{10.0f};
static float formant_x_{};
static float formant_y_{};
static float formant_shift_{};

static constexpr int kWidth = 500;
static constexpr int kHeight = 400;
static constexpr float kFs = 48000.0f;
static constexpr float kTwopi = std::numbers::pi_v<float> * 2.0f;

static void AudioInputCallback(void* _buffer, unsigned int frames) {
    struct T {
        float l;
        float r;
    };
    std::span buffer{reinterpret_cast<T*>(_buffer), frames};
    float const dsf_gain = dsf.NormalizeGain();
    for (auto& s : buffer) {
        auto x = dsf.Tick().real() * dsf_gain * output_gain;
        float sum = 0.0f;
        for (size_t i = 0; i < 5; ++i) {
            sum += filters[i].Tick(x) * gains[i];
        }
        s.l = sum;
        s.r = sum;
    }
}

static void UpdateFilters() {
    float xidx = formant_x_ * 4;
    xidx = std::clamp(xidx, 0.0f, 4.0f);
    float yidx = formant_y_ * 4;
    yidx = std::clamp(yidx, 0.0f, 4.0f);
    float freq_scale = std::exp2(formant_shift_ / 12.0f);

    auto& table = qwqdsp::filter::Formant::kFormants;
    qwqdsp::filter::RBJ design;
    for (size_t i = 0; i < 5; ++i) {
        size_t nowx = xidx;
        size_t nextx = std::min(nowx + 1, 4ull);
        float fracx = xidx - nowx;
        size_t nowy = yidx;
        size_t nexty = std::min(nowy + 1, 4ull);
        float fracy = yidx - nowy;

        auto line0 = table[nowx][nowy];
        auto line1 = table[nextx][nowy];
        float freq0 = std::lerp(line0.hz_freqs[i], line1.hz_freqs[i], fracx);
        float db0 = std::lerp(line0.db_amps[i], line1.db_amps[i], fracx);
        float bw0 = std::lerp(line0.hz_bws[i], line1.hz_bws[i], fracx);
        line0 = table[nowx][nexty];
        line1 = table[nextx][nexty];
        float freq1 = std::lerp(line0.hz_freqs[i], line1.hz_freqs[i], fracx);
        float db1 = std::lerp(line0.db_amps[i], line1.db_amps[i], fracx);
        float bw1 = std::lerp(line0.hz_bws[i], line1.hz_bws[i], fracx);

        float freq = std::lerp(freq0, freq1, fracy);
        float db = std::lerp(db0, db1, fracy);
        float bw = std::lerp(bw0, bw1, fracy);


        design.Bandpass(
            qwqdsp::convert::Freq2W(freq * freq_scale, kFs),
            q_);
        filters[i].Set(design.b0, design.b1, design.b2, design.a1, design.a2);
        gains[i] = std::pow(10.0f, db / 20.0f);
    }
}

int main(void) {
    InitWindow(kWidth, kHeight, "formant");

    InitAudioDevice();
    SetAudioStreamBufferSizeDefault(512);
    AudioStream stream = LoadAudioStream(48000, 32, 2);
    SetAudioStreamCallback(stream, AudioInputCallback);
    PlayAudioStream(stream);
    
    // formant滤波器界面
    Rectangle xypad_bound;
    xypad_bound.x = 0;
    xypad_bound.y = 0;
    xypad_bound.width = 400;
    xypad_bound.height = 400;
    Vector2 mouse_pos = GetMousePosition();
    float circle_x = 0.0f;
    float circle_y = 0.0f;

    Rectangle dsf_bound;
    dsf_bound.x = xypad_bound.x + xypad_bound.width + 50;
    dsf_bound.y = xypad_bound.y;
    dsf_bound.width = 50;
    dsf_bound.height = 50;

    Knob w0;
    w0.on_value_change = [](float v) {
        dsf.SetW0(v);
    };
    w0.set_bound(dsf_bound);
    w0.set_range(0.0f, 1.0f, 0.001f, 0.01f);
    w0.set_bg_color(BLACK);
    w0.set_fore_color(RAYWHITE);
    w0.set_title("w0");
    Knob w;
    w.on_value_change = [](float v) {
        dsf.SetWSpace(v);
    };
    dsf_bound.y += dsf_bound.height;
    w.set_bound(dsf_bound);
    w.set_range(0.0f, 1.0f, 0.001f, 0.01f);
    w.set_bg_color(BLACK);
    w.set_fore_color(RAYWHITE);
    w.set_title("w");
    Knob N;
    N.on_value_change = [](float v) {
        dsf.SetN(v);
    };
    dsf_bound.y += dsf_bound.height;
    N.set_bound(dsf_bound);
    N.set_range(1.0f, 512.0f, 1.0f, 256.0f);
    N.set_bg_color(BLACK);
    N.set_fore_color(RAYWHITE);
    N.set_title("n");
    Knob amp;
    amp.on_value_change = [](float v) {
        dsf.SetAmpFactor(v);
    };
    dsf_bound.y += dsf_bound.height;
    amp.set_bound(dsf_bound);
    amp.set_range(0.0f, 1.0f, 0.01f, 0.9f);
    amp.set_bg_color(BLACK);
    amp.set_fore_color(RAYWHITE);
    amp.set_title("amp");
    Knob q;
    q.on_value_change = [](float v) {
        q_ = v;
        UpdateFilters();
    };
    dsf_bound.y += dsf_bound.height;
    q.set_bound(dsf_bound);
    q.set_range(1.0f, 50.0f, 0.1f, 10.0f);
    q.set_bg_color(BLACK);
    q.set_fore_color(RAYWHITE);
    q.set_title("q");
    Knob output;
    output.on_value_change = [](float v) {
        output_gain = std::pow(10.0f, v / 20.0f);
    };
    dsf_bound.y += dsf_bound.height;
    output.set_bound(dsf_bound);
    output.set_range(-20.0f, 60.0f, 0.1f, 0.0f);
    output.set_bg_color(BLACK);
    output.set_fore_color(RAYWHITE);
    output.set_title("output");
    Knob shift;
    shift.on_value_change = [](float v) {
        formant_shift_ = v;
        UpdateFilters();
    };
    dsf_bound.y += dsf_bound.height;
    shift.set_bound(dsf_bound);
    shift.set_range(-24.0f, 24.0f, 0.1f, 0.0f);
    shift.set_bg_color(BLACK);
    shift.set_fore_color(RAYWHITE);
    shift.set_title("shift");

    UpdateFilters();

    SetTargetFPS(30);
    while (!WindowShouldClose()) {
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            auto const pos = GetMousePosition();
            if (CheckCollisionPointRec(pos, xypad_bound) && (pos.x != mouse_pos.x || pos.y != mouse_pos.y)) {
                mouse_pos = pos;
                float fx = (pos.x - xypad_bound.x) / xypad_bound.width;
                formant_x_ = fx;
                float fy = 1.0f - (pos.y - xypad_bound.y) / xypad_bound.height;
                formant_y_ = fy;
                UpdateFilters();
    
                circle_x = std::clamp(pos.x, xypad_bound.x, xypad_bound.x + xypad_bound.width);
                circle_y = std::clamp(pos.y, xypad_bound.y, xypad_bound.y + xypad_bound.height);
            }
        }

        BeginDrawing();
        {
            ClearBackground(BLACK);
            // 绘制xy版
            DrawRectangleLines(xypad_bound.x, xypad_bound.y, xypad_bound.width, xypad_bound.height, RAYWHITE);
            DrawCircleLines(circle_x, circle_y, 12.0f, RAYWHITE);

            w0.display();
            w.display();
            N.display();
            amp.display();
            q.display();
            output.display();
            shift.display();

            // 绘制字
            // x 类型 y 元音
            for (size_t i = 0; i < 5; ++i) {
                auto x = xypad_bound.x + i * (xypad_bound.width - 20.0f) / 5.0f + 20.0f;
                auto y = xypad_bound.y + xypad_bound.height - 20;
                DrawText(qwqdsp::filter::Formant::kTypeNames[i].data(), x, y, 12, RAYWHITE);
            }
            for (size_t i = 0; i < 5; ++i) {
                auto y = xypad_bound.y + i * (xypad_bound.height - 20.0f) / 4;
                auto x = xypad_bound.x;
                DrawText(qwqdsp::filter::Formant::kVowelNames[4 - i].data(), x, y, 20, RAYWHITE);
            }
        }
        EndDrawing();
    }

    UnloadAudioStream(stream);
    CloseAudioDevice();
    CloseWindow();
}

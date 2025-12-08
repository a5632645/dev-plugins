#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <span>

#include "raylib.h"
#include "../playing/slider.hpp"

#include "qwqdsp/oscillator/blit.hpp"
#include "qwqdsp/oscillator/blit_pwm.hpp"

static constexpr int kWidth = 500;
static constexpr int kHeight = 400;
static constexpr float kFs = 48000.0f;

enum Waveform {
    Impluse = 0,
    OddImpluse,
    Sawtooth,
    Square,
    Triangle,
    Sine,
    PwmImpluse,
    PWM,
    NumWaveforms
};
static constexpr const char* kWaveformNames[]{
    "impluse",
    "odd impluse",
    "saw",
    "square",
    "triangle",
    "sine",
    "pwm_impluse",
    "pwm"
};

static Waveform waveform = Waveform::Sawtooth;
static qwqdsp_oscillator::Blit dsp;
static qwqdsp_oscillator::BlitPWM dsp2;

static void AudioInputCallback(void* _buffer, unsigned int frames) {
    struct T {
        float l;
        float r;
    };
    std::span buffer{reinterpret_cast<T*>(_buffer), frames};

    switch (waveform) {
    case Impluse:
        for (auto& s : buffer) {
            s.l = dsp.Impluse();
            s.r = s.l;
        }
        break;
    case OddImpluse:
        for (auto& s : buffer) {
            s.l = dsp.OddImpluse();
            s.r = s.l;
        }
        break;
    case Sawtooth:
        for (auto& s : buffer) {
            s.l = dsp.Sawtooth();
            s.r = s.l;
        }
        break;
    case Square:
        for (auto& s : buffer) {
            s.l = dsp.Sqaure();
            s.r = s.l;
        }
        break;
    case Triangle:
        for (auto& s : buffer) {
            s.l = dsp.Triangle();
            s.r = s.l;
        }
        break;
    case Sine:
        for (auto& s : buffer) {
            s.l = dsp.Sine();
            s.r = s.l;
        }
        break;
    case PwmImpluse:
        for (auto& s : buffer) {
            s.l = dsp2.Impluse();
            s.r = s.l;
        }
        break;
    case PWM:
        for (auto& s : buffer) {
            s.l = dsp2.PWM();
            s.r = s.l;
        }
        break;
    default:
        assert(false);
    }
}

int main(void) {
    InitWindow(kWidth, kHeight, "simple blit oscillator");

    InitAudioDevice();
    SetAudioStreamBufferSizeDefault(512);
    AudioStream stream = LoadAudioStream(48000, 32, 2);
    SetAudioStreamCallback(stream, AudioInputCallback);
    PlayAudioStream(stream);
    
    Rectangle dsf_bound;
    dsf_bound.x = 0;
    dsf_bound.y = 0;
    dsf_bound.width = 50;
    dsf_bound.height = 50;

    Knob w;
    w.on_value_change = [](float v) {
        dsp.SetW(v);
        dsp2.SetW(v);
    };
    dsf_bound.y += dsf_bound.height;
    w.set_bound(dsf_bound);
    w.set_range(0.0f, 3.14f, 0.001f, 0.01f);
    w.set_bg_color(BLACK);
    w.set_fore_color(RAYWHITE);
    w.set_title("w");

    Knob N;
    N.on_value_change = [](float v) {
        dsp.SetNLimit(v);
        dsp2.SetNLimit(v);
    };
    dsf_bound.y += dsf_bound.height;
    N.set_bound(dsf_bound);
    N.set_range(1.0f, 1024.0f, 1.0f, 5.0f);
    N.set_bg_color(BLACK);
    N.set_fore_color(RAYWHITE);
    N.set_title("n");

    Knob amp;
    amp.on_value_change = [](float v) {
        dsp.SetAmp(v);
        dsp2.SetAmp(v);
    };
    dsf_bound.y += dsf_bound.height;
    amp.set_bound(dsf_bound);
    amp.set_range(0.01f, 0.99f, 0.01f, 0.5f);
    amp.set_bg_color(BLACK);
    amp.set_fore_color(RAYWHITE);
    amp.set_title("amp");
    
    Knob pwm;
    pwm.on_value_change = [](float v) {
        dsp2.SetPWM(v);
    };
    dsf_bound.y += dsf_bound.height;
    pwm.set_bound(dsf_bound);
    pwm.set_range(0.01f, 0.99f, 0.01f, 0.5f);
    pwm.set_bg_color(BLACK);
    pwm.set_fore_color(RAYWHITE);
    pwm.set_title("pwm");

    dsf_bound.y += dsf_bound.height;
    auto waveform_bound = dsf_bound;
    waveform_bound.width = GetScreenWidth();
    waveform_bound.height = 30;
    float each_width = waveform_bound.width / static_cast<float>(NumWaveforms);
    size_t num_waveforms = static_cast<size_t>(NumWaveforms);
    
    SetTargetFPS(30);
    while (!WindowShouldClose()) {
        BeginDrawing();
        {
            ClearBackground(BLACK);

            w.display();
            N.display();
            amp.display();
            pwm.display();

            auto mouse_pos = GetMousePosition();
            for (size_t i = 0; i < num_waveforms; ++i) {
                float x = i * each_width;
                float y = waveform_bound.y;
                float w = each_width - 2;
                float h = waveform_bound.height;
                if (CheckCollisionPointRec(mouse_pos, Rectangle{x,y,w,h}) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    waveform = static_cast<Waveform>(i);
                }

                if (i != waveform) {
                    DrawRectangleLines(x, y, w, h, WHITE);
                    DrawText(kWaveformNames[i], x, y, 12, WHITE);
                }
                else {
                    DrawRectangle(x, y, w, h, WHITE);
                    DrawText(kWaveformNames[i], x, y, 12, BLACK);
                }
            }
        }
        EndDrawing();
    }

    UnloadAudioStream(stream);
    CloseAudioDevice();
    CloseWindow();
}

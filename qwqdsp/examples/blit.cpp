#include <algorithm>
#include <array>
#include <cstddef>
#include <span>

#include "raylib.h"
#include "../playing/slider.hpp"

#include "qwqdsp/osciilor/blit.hpp"

static constexpr int kWidth = 500;
static constexpr int kHeight = 400;
static constexpr float kFs = 48000.0f;

static qwqdsp::oscillor::Blit dsp;

static void AudioInputCallback(void* _buffer, unsigned int frames) {
    struct T {
        float l;
        float r;
    };
    std::span buffer{reinterpret_cast<T*>(_buffer), frames};
    for (auto& s : buffer) {
        s.l = dsp.Triangle();
        s.r = s.l;
    }
}

int main(void) {
    InitWindow(kWidth, kHeight, "formant");

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
    };
    dsf_bound.y += dsf_bound.height;
    w.set_bound(dsf_bound);
    w.set_range(0.0f, 3.14f, 0.001f, 0.01f);
    w.set_bg_color(BLACK);
    w.set_fore_color(RAYWHITE);
    w.set_title("w");
    Knob N;
    N.on_value_change = [](float v) {
        // dsp.SetN(v);
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
    };
    dsf_bound.y += dsf_bound.height;
    amp.set_bound(dsf_bound);
    amp.set_range(0.01f, 0.99f, 0.01f, 0.5f);
    amp.set_bg_color(BLACK);
    amp.set_fore_color(RAYWHITE);
    amp.set_title("amp");
    Knob pwm;
    pwm.on_value_change = [](float v) {
        // dsp.SetPWM(v);
    };
    dsf_bound.y += dsf_bound.height;
    pwm.set_bound(dsf_bound);
    pwm.set_range(0.01f, 0.99f, 0.01f, 0.5f);
    pwm.set_bg_color(BLACK);
    pwm.set_fore_color(RAYWHITE);
    pwm.set_title("pwm");
    
    SetTargetFPS(30);
    while (!WindowShouldClose()) {
        BeginDrawing();
        {
            ClearBackground(BLACK);

            w.display();
            N.display();
            amp.display();
            pwm.display();
        }
        EndDrawing();
    }

    UnloadAudioStream(stream);
    CloseAudioDevice();
    CloseWindow();
}
